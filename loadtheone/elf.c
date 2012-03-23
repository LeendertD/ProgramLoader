#include <stdio.h>
#include <stdlib.h>

#include "ELF.h"
#include "basfunc.h"
#include "loader.h"


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

void elf_loadit(char *dstart, size_t size, Elf_Addr base, int verbose){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  struct Elf_Phdr *phdr = (struct Elf_Phdr*) (dstart + ehdr->e_phoff);
  
  Elf_Half i;
  // Then copy the LOAD segments into their right locations
  for (i=0; i < ehdr->e_phnum; ++i){
    if (phdr[i].p_type == PT_LOAD && phdr[i].p_memsz > 0){
      Verify(phdr[i].p_memsz >= phdr[i].p_filesz,
          "file has an invalid segment");
            
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
        // We do not reserve here because this
        // will be taken care of by the ActiveROM during loading.
        if (verbose){
          //LOADING HERE SEEMS NEEDED
          fprintf(stderr, "Loader %d bytes loaded at 0x%x\n",phdr[i].p_filesz, phdr[i].p_vaddr);
          //clog << msg_prefix << ": " << dec << phdr[i].p_filesz << " bytes loadable at virtual address 0x" << hex << phdr[i].p_vaddr << endl;
        }
      } else {
        //segment that needs reservation, 2 types exist, init and non init
        reserve_range((void*)phdr[i].p_vaddr, phdr[i].p_memsz, perm);
        //memory.Reserve(phdr[i].p_vaddr, phdr[i].p_memsz, perm);
        if (verbose){
          fprintf(stderr, "Loader reserved %d bytes at 0x%x\n", phdr[i].p_memsz, phdr[i].p_vaddr);
          //clog << msg_prefix << ": " << dec << phdr[i].p_memsz << " bytes reserved at virtual address 0x" << hex << phdr[i].p_vaddr << endl;
        }
      }
    }
  }
  if (verbose){
    const char* type = (ehdr->e_machine == MACHINE_LEGACY)? "legacy":"microthreaded";
      
    fprintf(stderr, "Loaded %s ELF binary with virtual base 0x%x entry point 0x%x\n",
        type, base, ehdr->e_entry);
    //clog << msg_prefix << ": loaded " << type << " ELF binary with virtual base address 0x" << hex << base
    //     << ", entry point at 0x" << hex << ehdr.e_entry << endl;
  }

  //RUN THE THING

  //return make_pair(ehdr.e_entry, ehdr.e_machine == MACHINE_LEGACY);


}

// Load the program image into the memory
int elf_loadprogram(char* data, size_t size, int verbose){
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

  //Is this Allignment? won't it damage something?
  static const int PAGE_SIZE = 4096;
  base = base & -PAGE_SIZE;

  elf_loadit(data, size, base, verbose);

}

