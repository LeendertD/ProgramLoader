#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "ELF.h"
#include "basfunc.h"
#include "loader.h"
#include <unistd.h>

int elf_header_marshall(char *dstart, size_t size){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;

  Verify(size >= sizeof(struct Elf_Ehdr),
      "ELF file too short or truncated");
  // Unmarshall header
  ehdr->e_type      = elftohh(ehdr->e_type);
  ehdr->e_machine   = elftohh(ehdr->e_machine);
  ehdr->e_version   = elftohw(ehdr->e_version);
  ehdr->e_entry     = elftoha(ehdr->e_entry);
  ehdr->e_phoff     = elftoho(ehdr->e_phoff);
  ehdr->e_shoff     = elftoho(ehdr->e_shoff);
  ehdr->e_flags     = elftohw(ehdr->e_flags);
  ehdr->e_ehsize    = elftohh(ehdr->e_ehsize);
  ehdr->e_phentsize = elftohh(ehdr->e_phentsize);
  ehdr->e_phnum     = elftohh(ehdr->e_phnum);
  ehdr->e_shentsize = elftohh(ehdr->e_shentsize);
  ehdr->e_shnum     = elftohh(ehdr->e_shnum);
  ehdr->e_shstrndx  = elftohh(ehdr->e_shstrndx);
  return 0;
}

int elf_header_check(char *dstart, size_t size){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  // Check file signature
  if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
         ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
         ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
         ehdr->e_ident[EI_MAG3] == ELFMAG3){
    return 0;
  } else {
    fprintf(stderr, "invalid ELF file signature");
    return -1;
  }
}

int elf_header_check_arch(char *dstart, size_t size){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  // Check that this file is for our 'architecture'
  if (ehdr->e_ident[EI_VERSION] != EV_CURRENT){
    fprintf(stderr, "ELF version mismatch");
    return -1;
  }
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS){
    fprintf(stderr, "file is not of proper bitsize");
    return -1;
  }
  if (ehdr->e_ident[EI_DATA] != ELFDATA){
    fprintf(stderr, "file is not of proper endianness");
    return -1;
  }
  if (! (ehdr->e_machine == MACHINE_NORMAL ||
         ehdr->e_machine == MACHINE_LEGACY)){
    fprintf(stderr, "target architecture is not supported");
    return -1;
  }
  if (ehdr->e_type != ET_EXEC){
    fprintf(stderr, "file is not an executable file");
    return -1;
  }
  if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0){
    fprintf(stderr, "file has no program header");
    return -1;
  }
  if (ehdr->e_phentsize != sizeof(struct Elf_Phdr)){
    fprintf(stderr, "file has an invalid program header");
    return -1;
  }
  if (! (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize <= size)){
    fprintf(stderr, "file has an invalid program header");
    return -1;
  }
  return 0;
}
void elf_pheader_marshall(struct Elf_Phdr *phdr){
  phdr->p_type   = elftohw (phdr->p_type);
  phdr->p_flags  = elftohw (phdr->p_flags);
  phdr->p_offset = elftoho (phdr->p_offset);
  phdr->p_vaddr  = elftoha (phdr->p_vaddr);
  phdr->p_paddr  = elftoha (phdr->p_paddr);
  phdr->p_filesz = elftohxw(phdr->p_filesz);
  phdr->p_memsz  = elftohxw(phdr->p_memsz);
  phdr->p_align  = elftohxw(phdr->p_align);
}

Elf_Addr elf_findbase_marshallphdr(char *dstart, size_t size){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  struct Elf_Phdr *phdr = (struct Elf_Phdr*) (dstart + ehdr->e_phoff);

  // Determine base address and check for loadable segments
  int hasLoadable = 0;
  Elf_Addr base = 0;
  Elf_Half i;
  for (i=0; i < ehdr->e_phnum; ++i){
    elf_pheader_marshall(phdr + i);
    if (phdr[i].p_type == PT_LOAD){
      if ( (!hasLoadable) || (phdr[i].p_vaddr < base)) {
        base = phdr[i].p_vaddr; 
      }
      hasLoadable = 1;
    }
  }
  // Commented out in original code:
  // Verify(hasLoadable, "file has no loadable segments");
  return base;
}

int elf_loadit(char *dstart, size_t size, Elf_Addr base, int verbose){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  struct Elf_Phdr *phdr = (struct Elf_Phdr*) (dstart + ehdr->e_phoff);
  
  Elf_Half i;

  if (verbose) fprintf(stderr, "Trying to load: %p size %p @ %p\n", (void*)dstart, (void*)size, (void*)base);

  // Then copy the LOAD segments into their right locations
  for (i=0; i < ehdr->e_phnum; ++i){
    if (phdr[i].p_type == PT_LOAD && phdr[i].p_memsz > 0){
      Verify(phdr[i].p_memsz >= phdr[i].p_filesz,
          "file has an invalid segment");
      char *act_addr;
            
      int perm = 0;
      if (phdr[i].p_flags & PF_R) perm |= perm_read;
      //perm |= IMemory::PERM_READ|IMemory::PERM_DCA_READ;
      if (phdr[i].p_flags & PF_W) perm |= perm_write;
      //IMemory::PERM_WRITE|IMemory::PERM_DCA_WRITE;
      if (phdr[i].p_flags & PF_X) perm |= perm_exec;
      //IMemory::PERM_EXECUTE;
            
      if (phdr[i].p_filesz > 0){
        //Segment containing payload
        Verify(phdr[i].p_offset + phdr[i].p_filesz <= size,
            "file has an invalid segment");
        /*ActiveROM::LoadableRange r;
        r.rom_offset = phdr[i].p_offset;
        r.rom_size = phdr[i].p_filesz;
        r.vaddr = phdr[i].p_vaddr;
        r.vsize = phdr[i].p_memsz;
        r.perm = (IMemory::Permissions)perm;
        ranges.push_back(r);
        */

        act_addr = ((char*)base) + phdr[i].p_vaddr;

        if (verbose) fprintf(stderr, "load F: %d_%d: size %p,%p @ %p with %d\n", phdr[i].p_type, i,
          (void*)phdr[i].p_memsz, (void*)phdr[i].p_filesz, act_addr, perm);
        //reserve, prepare data
        reserve_range(act_addr, phdr[i].p_memsz, perm);
        memcpy(act_addr, dstart + phdr[i].p_offset, phdr[i].p_filesz);
        if (phdr[i].p_filesz < phdr[i].p_memsz){
          //beyond the supplied data, 0 as per spec
          memset(act_addr + phdr[i].p_filesz, 0, phdr[i].p_memsz - phdr[i].p_filesz-1);
        }

        //LOADING HERE SEEMS NEEDED
        if (verbose){
          fprintf(stderr, "Loader %p bytes loaded at %p\n",(void*) phdr[i].p_filesz,(void*) act_addr);
          //clog << msg_prefix << ": " << dec << phdr[i].p_filesz << " bytes loadable at virtual address 0x" << hex << phdr[i].p_vaddr << endl;
        }
      } else {
        act_addr = ((char*)base) + phdr[i].p_vaddr;

        if (verbose) fprintf(stderr, "load U: %d: %p size %p,%p @ %p\n with %d", phdr[i].p_type, i,
          (void*)phdr[i].p_memsz, (void*)phdr[i].p_filesz, act_addr, perm);
        //segment that needs reservation, 2 types exist?
        reserve_range((void*)act_addr, phdr[i].p_memsz, perm);
        
        //memory.Reserve(phdr[i].p_vaddr, phdr[i].p_memsz, perm);
        if (verbose){
          fprintf(stderr, "Loader reserved %p bytes at %p\n",(void*) phdr[i].p_memsz,(void*) act_addr);
          //clog << msg_prefix << ": " << dec << phdr[i].p_memsz << " bytes reserved at virtual address 0x" << hex << phdr[i].p_vaddr << endl;
        }
      }
    }
  }
  if (verbose){
    const char* type = (ehdr->e_machine == MACHINE_LEGACY)? "legacy":"microthreaded";
      
    fprintf(stderr, "Loaded %s ELF binary with virtual base %p entry point %p\n",
        type, (void*)base, (void*)base + ehdr->e_entry);
  }


  //return make_pair(ehdr.e_entry, ehdr.e_machine == MACHINE_LEGACY);

  return 0;
}

int elf_runpatches(char *mstart, size_t size, int verbose,
                   int patch_fd){
  char abuffer[128] = {0};
  char mbuffer[128] = {0};
  char vbuffer[128] = {0};
  int state = 0;
  int cnt;

  if (patch_fd == -1) return 0;
  if (verbose) fprintf(stderr, "Patchfile found\n");

  while (1){
    char *tgt = NULL;
    char buf;
  
    if (1 != read(patch_fd, &buf, 1)){
      if (verbose) perror("End of patchfile, or error\n");
      return 0;
    }
    switch (state){
      case 0:
        tgt = abuffer + cnt++;
        if (buf == '='){
          state++;
          cnt = 0;
          break;
        }
        tgt[0] = buf;
        tgt[1] = 0;
        break;
      case 1:
        tgt = mbuffer + cnt++;
        if (buf == '='){
          state++;
          cnt = 0;
          break;
        }
        tgt[0] = buf;
        tgt[1] = 0;
        break;
      case 2:
        tgt = vbuffer + cnt++;
        if (buf == '\n'){
          uint64_t pnt = 0;
          Elf_Addr addr_start = 0;
          unsigned char *dbuf = NULL;
          size_t len_size;
          char *cr;
          unsigned char *dr;
          pnt = strtoul(abuffer, NULL,0);
          addr_start = pnt;
          pnt = strtoul(mbuffer, NULL,0);
          len_size = pnt;
          dbuf = malloc(len_size + 8 /*dirty fix for printing the first 8 bytes*/);

          dr = dbuf;
          cr = vbuffer;
          while (pnt > 0){
            char *nc = NULL;
            *dr = (unsigned char) strtoul(cr, &nc, 0);
            dr++;
            if (nc[0] == ' '){
              cr = nc + 1;
            } else if (nc[0] == 0){
              break;
            } else {
              cr = nc;
            }
            pnt--;
          }

          state = 0;
          cnt = 0;
          //PATCHME
          if (verbose) fprintf(stderr, "Patching. %s<%s> -> %s\n", abuffer, mbuffer, vbuffer);
          if (verbose) fprintf(stderr, "Patching: %p<%p> -> %p\n", (void*)addr_start, (void*)len_size,
              (void*)(* (uint64_t*)dbuf));
          patcher_patch(mstart + addr_start, 1, dbuf, len_size);
          free(dbuf);
          abuffer[0] = mbuffer[0] = vbuffer[0] = 0;
          break;
        }
        tgt[0] = buf;
        tgt[1] = 0;
        break;
      default:
        return 1;
    }
  }

  return 0;
}


int elf_spawn(char *dstart, size_t size, Elf_Addr base, 
              int verbose){

  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  if (verbose) fprintf(stderr, "Spawning program\n");
  //RUN THE THING
  function_spawn((main_function_t*) ((char*)base + ehdr->e_entry));
  //function_spawn((main_function_t*) ((char*)base + /*FAKE ENTRY, MAIN*/ 0x1000004));
  return 0;
}

// Load the program image into the memory
int elf_loadprogram(char* data, size_t size, int verbose,
                    int patch_fd){
  Elf_Addr base;
  
  if (elf_header_marshall(data,size)){
    fprintf(stderr, "Marshalling failed\n");
    return -1;
  }
  if (elf_header_check(data,size)){
    fprintf(stderr, "Header checking failed\n");
    return -1;
  }
  if (elf_header_check_arch(data,size)){
    fprintf(stderr, "Architecture check failed\n");
    return -1;
  }

  base = elf_findbase_marshallphdr(data, size);
  if (verbose) fprintf(stderr, "base_file: %p\n", (void*)base);
  //Is this Allignment? won't it damage something?
  static const int PAGE_SIZE = 4096;
  base = base & -PAGE_SIZE;
  base = base | 0x1000000000000000;//Move it out of existing mem
  if (verbose) fprintf(stderr, "base_used: %p\n", (void*)base);



  if (elf_loadit(data, size, base, verbose)){
    fprintf(stderr, "Elf loading failed\n");
    return -1;
  }
  if (elf_runpatches((char*)base, size, verbose, patch_fd)){
    fprintf(stderr, "Patching problem\n");
    return -1;
  }
  
  if (elf_spawn(data, size, base, verbose)){
    fprintf(stderr, "Elf spawning failed\n");
    return -1;
  }


  return  0;
}

