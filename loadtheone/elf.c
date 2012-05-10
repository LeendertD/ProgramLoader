#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <errno.h>
#include "ELF.h"
#include "basfunc.h"
#include "loader.h"
#include <unistd.h>

#define NODE_BASELOCK 3

#define MAXPROCS 1024

//Big table of structs
struct admin_s proctable[MAXPROCS];
//Index in the array which should be free
int nextfreepid = 1;

//Init the array, once should be quite enough
void init_admins(void){
  int i;
  for (i=0;i<MAXPROCS;i++){
    proctable[i].base = 0;
    proctable[i].pidnum = 0;
    proctable[i].argc = 0;
    proctable[i].argv = NULL;
    proctable[i].envp = NULL;

    proctable[i].nextfreepid = i+1;
  }
  proctable[MAXPROCS-1].nextfreepid = 0;
}

sl_def(slbase_fn,, sl_glparm(struct admin_s**, basep)){
  int npid = nextfreepid;
  struct admin_s **val = sl_getp(basep);
  *val = &proctable[npid];
  (*val)->base = npid * base_off;
  (*val)->pidnum = npid;
  
  nextfreepid = (*val)->nextfreepid;
  (*val)->nextfreepid = 0;;
}
sl_enddef

sl_def(sldelbase_fn,, sl_glparm(int, deadpid)){
  int npid = nextfreepid;
  int deadpid = sl_getp(deadpid);
  struct admin_s *val = &proctable[deadpid];
  val->pidnum = 0;
  
  val->nextfreepid = npid;
  nextfreepid = deadpid;
}
sl_enddef


Elf_Addr locked_newbase(struct admin_s **params){
  sl_create(, MAKE_CLUSTER_ADDR(NODE_BASELOCK, 1) ,,,,, sl__exclusive, slbase_fn, sl_glarg(struct admin_s**, params, params));
  sl_sync();

  return (*params)->base;
}

void locked_delbase(int deadpid){
  sl_create(, MAKE_CLUSTER_ADDR(NODE_BASELOCK, 1) ,,,,, sl__exclusive, sldelbase_fn, sl_glarg(int, deadpid, deadpid));
  sl_sync();
}

#define SANE_SIZE sizeof(struct Elf_Ehdr)

int elf_loadfile_p(struct admin_s * params, enum e_settings flags){
  int fin = -1;
  size_t fsize = 0;
  struct stat fstatus;
  void *fdata = NULL;
  size_t r;
  size_t sr;
  off_t toread;
  int verbose = flags & e_verbose;
  char buff[1024];

  if (verbose) {
    snprintf(buff, 1023,"Loading %s\n", params->fname);
    locked_print_string(buff, PRINTERR);
  }
  fin = open(params->fname, O_RDONLY);

  if (-1 == fin){
    const char *err = strerror(errno);
    snprintf(buff, 1023, "File could not be opened: %s\n", err);
    locked_print_string(buff, PRINTERR);
    return 0;
  }
  if (fstat(fin, &fstatus)) {
    const char *err = strerror(errno);
    snprintf(buff, 1023, "File not statted %s: %s\n", params->fname, err);
    locked_print_string(buff, PRINTERR);
    return 0;
  }
  fsize = fstatus.st_size;
  if (fsize < SANE_SIZE){
    locked_print_string("Filesize too small, not a valid file\n", PRINTERR);
    return 0;
  }
  if (verbose) locked_print_string("File opened\n", PRINTERR);
  fdata = malloc(fsize);

  //current offset
  sr = 0;
  toread = fsize;
  while (toread > 0){
    errno = 0;
    r = read(fin, fdata + sr, fsize);
    if (r != fsize){
      if (errno == 0){//No error, incomplete read
        toread -= r;
        sr += r;
        continue;
      }
      const char *err = strerror(errno);
      snprintf(buff, 1023, "Read error: %s, %d of %d read\n",
                       err, (int)r, (int)fsize);
      locked_print_string(buff, PRINTERR);
      return 0;
    }
  }
  if (verbose) locked_print_string("File read\n", PRINTERR);

  /*Data done, close file*/
  if (verbose) locked_print_string("Closing file\n", PRINTERR);
  close(fin);
  fin = -1;
  /*File closed*/

  if (elf_loadprogram_p(fdata, fsize, verbose,
        flags,
        params
        )){
    locked_print_string("Elf failure\n", PRINTERR);
  }
  
  free(fdata);
  return 0;
}

int elf_loadfile(const char *fname, enum e_settings flags,
              int argc, char **argv, char* env){

  struct admin_s params;
  params.fname = strdup(fname);
  params.base = 0;
  params.core_start = -1;
  params.core_size = -1;

  params.argc = argc;
  params.argv = argv;
  params.envp = env;

  return elf_loadfile_p(&params, flags);
}

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

  if ((dstart + size) < (char*)(& ehdr->e_ident[EI_MAG3])) {
    locked_print_string("Header problem, size too low\n", PRINTERR);
    return -1; // Data too short for reading
  }

  // Check file signature
  if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
         ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
         ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
         ehdr->e_ident[EI_MAG3] == ELFMAG3){
    return 0;
  } else {
    locked_print_string("invalid ELF file signature", PRINTERR);
    return -1;
  }
}

int elf_header_check_arch(char *dstart, size_t size){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  // Check that this file is for our 'architecture'
  if (ehdr->e_ident[EI_VERSION] != EV_CURRENT){
    locked_print_string("ELF version mismatch", PRINTERR);
    return -1;
  }
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS){
    locked_print_string("file is not of proper bitsize", PRINTERR);
    return -1;
  }
  if (ehdr->e_ident[EI_DATA] != ELFDATA){
    locked_print_string("file is not of proper endianness", PRINTERR);
    return -1;
  }
  if (! (ehdr->e_machine == MACHINE_NORMAL ||
         ehdr->e_machine == MACHINE_LEGACY)){
    locked_print_string("target architecture is not supported", PRINTERR);
    return -1;
  }

#ifdef CHECK_EXECELF_ENABLETHISTOBREAKSTUFF
  if (ehdr->e_type != ET_EXEC){
    locked_print_string("file is not an executable file", PRINTERR);
    return -1;
  }
#endif
  if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0){
    locked_print_string("file has no program header", PRINTERR);
    return -1;
  }
  if (ehdr->e_phentsize != sizeof(struct Elf_Phdr)){
    locked_print_string("file has an invalid program header", PRINTERR);
    return -1;
  }
  if (! (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize <= size)){
    locked_print_string("file has an invalid program header", PRINTERR);
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

  (void) size; // A check could be added

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

int elf_loadit(char *dstart, size_t size, struct admin_s* adminstart, int verbose){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  struct Elf_Phdr *phdr = (struct Elf_Phdr*) (dstart + ehdr->e_phoff);
  Elf_Addr base = adminstart->base;
  
  Elf_Half i;
  char buff[1024];

  if (verbose) {
    char buff[1024];
    snprintf(buff, 1023,
        "Trying to load program: %p size %p @ %p\n",
        (void*)dstart, (void*)size, (void*)base);
    locked_print_string(buff, PRINTERR);
  }

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
            
      act_addr = ((char*)base) + phdr[i].p_vaddr;
      if (phdr[i].p_filesz > 0){
        //Segment containing payload
        Verify(phdr[i].p_offset + phdr[i].p_filesz <= size,
            "file has an invalid segment");

        if (verbose) {
          char buff[1024];
          snprintf(buff, 1023,
              "load F: %d_%d: size %p,%p @ %p with %d\n",
              phdr[i].p_type, i, (void*)phdr[i].p_memsz,
              (void*)phdr[i].p_filesz, act_addr, perm);
          locked_print_string(buff, PRINTERR);
        }
        //reserve, prepare data
#ifndef FLAG_NONEEDODDPERMS
        reserve_range(act_addr, phdr[i].p_memsz, perm |
            perm_read|perm_write|perm_exec
            );
#else
        reserve_range(act_addr, phdr[i].p_memsz, perm);
#endif
        memcpy(act_addr, dstart + phdr[i].p_offset, phdr[i].p_filesz);
        if (phdr[i].p_filesz < phdr[i].p_memsz){
          //beyond the supplied data, 0 as per spec
          memset(act_addr + phdr[i].p_filesz, 0, phdr[i].p_memsz - phdr[i].p_filesz-1);
        }

        //LOADING HERE SEEMS NEEDED
        if (verbose){
          char buff[1024];
          snprintf(buff, 1023,"Loader %p bytes loaded at %p\n",(void*) phdr[i].p_filesz,(void*) act_addr);
          locked_print_string(buff, PRINTERR);
        }
      } else {
        /* If the name equals a 'magic' name, remember the address for usage for
         * things like argv, env. Or anything yet to be invented.
         **/
        if (verbose) {
          snprintf(buff, 1023, "load U: %d: 0X%x size %p,%p @ %p\n with %d", phdr[i].p_type, i,
          (void*)phdr[i].p_memsz, (void*)phdr[i].p_filesz, act_addr, perm);
          locked_print_string(buff, PRINTERR);
        }
        //segment that needs reservation, 2 types exist?
        reserve_range((void*)act_addr, phdr[i].p_memsz, perm);
        
        //memory.Reserve(phdr[i].p_vaddr, phdr[i].p_memsz, perm);
        if (verbose){
          snprintf(buff, 1023, "Loader reserved %p bytes at %p\n",(void*) phdr[i].p_memsz,(void*) act_addr);
          locked_print_string(buff, PRINTERR);
        }
      }
    }
  }
  if (verbose){
    const char* type = (ehdr->e_machine == MACHINE_LEGACY)? "legacy":"microthreaded";
      
    snprintf(buff, 1023, "Loaded %s ELF binary with virtual base %p entry point %p\n",
        type, (void*)base, (void*)base + ehdr->e_entry);
    locked_print_string(buff, PRINTERR);
  }

  return 0;
}

const char *elf_sectiontype(Elf_Addr in){
  switch (in){
    case SECTION_NULL:
      return "NULLTYPE";
    break;
    case SECTION_PROGBITS:
      return "Progbits";
    break;
    case SECTION_SYMTAB:
      return "Symtab";
    break;
    case SECTION_STRTAB:
      return "Strtab";
    break;
    case SECTION_RELA:
      return "Rela";
    break;
    case SECTION_HASH:
      return "Hash";
    break;
    case SECTION_DYN:
      return "Dynamic";
    break;
    case SECTION_NOTE:
      return "Note";
    break;
    case SECTION_NOBITS:
      return "Nobits";
    break;
    case SECTION_REL:
      return "Rel";
    break;
    case SECTION_SHLIB:
      return "Shlib";
    break;
    case SECTION_DYNSYM:
      return "Dynsym";
    break;
  }
  if ((in >= SECTION_LOPROC) && (in <= SECTION_HIPROC)) return "Proc";
  if ((in >= SECTION_LOUSER) &&  (in <= SECTION_HIUSER)) return "User";

  return "UNKNOWN";
}

const char * elf_symname(Elf_Addr base,struct Elf_Shdr* s, struct Elf_Sym* sym,struct Elf_Ehdr *ehdr){
  const char * stringdata = (const char*) ((struct Elf_Shdr*) (base + ehdr->e_shoff + (s->sh_link * ehdr->e_shentsize)))->sh_offset + base;
  return stringdata + sym->st_name;
}

const char *elf_sectname(Elf_Addr base, int num,struct Elf_Ehdr *ehdr){
  const char *stringdata = (const char *)((struct Elf_Shdr*) (base + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize)))->sh_offset + base;
  return(num + stringdata);
}

int elf_sectionscan(char *dstart, size_t size, struct admin_s* adminstart, int verbose){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  Elf_Addr base = adminstart->base;
  Elf_Half sectsize = ehdr->e_shentsize;
  Elf_Half numsects = ehdr->e_shnum;
  Elf_Off sectoff = ehdr->e_shoff;
  Elf_Half strndx = ehdr->e_shstrndx;
  
  int relocs[1024] = {0};
  int nr_relocs = 0;
  int symtabind = 0;
  int symbolsize = 0;
  struct Elf_Sym*  symbase = NULL;
  struct Elf_Shdr*  symsect = NULL;
  Elf_Half i;
  char buff[1024];
  if (sectsize != sizeof(struct Elf_Shdr)) locked_print_string("SIZE MISMATCH\n", PRINTERR);



  snprintf(buff,1023, "Got %d sections of size %d each Expecting size %lu\n"\
                      "Stringtable in section %d\n",
      numsects, sectsize, sizeof(struct Elf_Shdr), strndx);
  locked_print_string(buff, PRINTERR);

  snprintf(buff,1023, "\nSection %3s(%8s): %15s,%6s %14s,%14s,%14s,%14s, %6s,%6s, %14s,%14s\n", "#","Type",
        "Name",
        "Type#", "Flags", "Addr", "Off", "Size", "Link", "Info", "Align", "Entsize");
  locked_print_string(buff, PRINTERR);

  for (i=0;i<numsects;i++){
    //Loop over the sections, check their need for further processing.
    struct Elf_Shdr *s = (struct Elf_Shdr*) (dstart + ehdr->e_shoff + (i*sectsize));
    snprintf(buff,1023, "Section %3d(%8s): %15s,%6u, %14lu,%14lu,%14lu,%14lu, %6u,%6u, %14lu,%14lu\n", i,elf_sectiontype(s->sh_type),
        elf_sectname((Elf_Addr)dstart, s->sh_name, ehdr),
        s->sh_type, s->sh_flags, s->sh_addr, s->sh_offset, s->sh_size, s->sh_link, s->sh_info, s->sh_addralign, s->sh_entsize);
    locked_print_string(buff, PRINTERR);
    switch (s->sh_type){
//    case SECTION_STRTAB:{
//      int r = 1;
//      snprintf(buff,1023, "Stringtable %u, %lu@%p\n",s->sh_name, s->sh_size, (void*)s->sh_offset);
//      
//      while (dstart[s->sh_offset + r]){
//        int nr = 0;
//        snprintf(buff,1023, "%s%n\n", dstart + s->sh_offset + r, &nr);;
//        locked_print_string(buff, PRINTERR);
//        r += nr+1;
//        if (!nr) break;
//      }
//      //Scan for 'arg storage' section?
//      break;}

      case SECTION_PROGBITS:
        if (streq(elf_sectname((Elf_Addr)dstart, s->sh_name, ehdr),".argroom")){
          if (verbose) locked_print_string("Found the argument section\n", PRINTERR);
          //adminstart->argroom_offset = s->sh_offset;
          adminstart->argroom_size = s->sh_size;
          //TODO: makes sense?
        }
        break;

      case SECTION_DYNSYM:
        if (verbose) locked_print_string("Dynsym:\n", PRINTERR); 
      case SECTION_SYMTAB:{
        if (SECTION_DYNSYM == s->sh_type){
        if (verbose) locked_print_string("Setting symbol pointer\n", PRINTERR); 
          //Note the section, needed later
          if (symtabind) locked_print_string("Double symtab\n", PRINTERR);
          symtabind = i;
          symbolsize = s->sh_entsize;
          symbase = (struct Elf_Sym*) (dstart + s->sh_offset);
          symsect = (struct Elf_Shdr*) s;
        }

        unsigned  int r=0;
        int c = 0;
        if (s->sh_entsize != sizeof(struct Elf_Sym)) locked_print_string("Size mismatch symtab\n", PRINTERR);
        while (r < s->sh_size){
          struct Elf_Sym *symt = (struct Elf_Sym*) (dstart + s->sh_offset + r);
          int bind = ELF_SYM_BIND(symt->st_info);
          int type = ELF_SYM_TYPE(symt->st_info);
          snprintf(buff, 1023, "Sym %3d: %15s(%d) %17p of size %5lu, %3d<%2d,%2d> %3d, %8d\n", c,
              elf_symname((Elf_Addr)dstart, s, symt,ehdr),
              symt->st_name,
              (void*)symt->st_value,
              symt->st_size,
              symt->st_info,
              bind,type,
              symt->st_other,
              symt->st_shndx
              );
          locked_print_string(buff, PRINTERR);
          c++;
          r += s->sh_entsize;
        }
        break;
      }
      case SECTION_RELA:{
        //Note the needed relocs
        relocs[nr_relocs++] = i;
        if (verbose) locked_print_string("Found the RELA section\n", PRINTERR);
        break;}
      case SECTION_REL:{
        if (verbose) locked_print_string("Found the REL section\n", PRINTERR);
        break;}
      default:
        //Nothing of interest
        break;
    }
  }

  for (i=0;i<nr_relocs;i++){
    struct Elf_Shdr *s = (struct Elf_Shdr*) (dstart + ehdr->e_shoff + (relocs[i] * sectsize));
    unsigned int r=0;
    if (s->sh_entsize != sizeof(struct Elf_Rela)) locked_print_string("Size mismatch rela\n", PRINTERR);
    while (r < s->sh_size){
      struct Elf_Rela *rela = (struct Elf_Rela*)(dstart + s->sh_offset + r);
      Elf_Addr * vicloc = (Elf_Addr*) (adminstart->base + rela->r_offset); //add section? relative to what be yeolde offset
      Elf_Addr newval = 0;
      Elf_Addr sectloc = *((Elf_Addr*) (adminstart->base + s->sh_offset));
      int rtype = ELF_REL_TYPE(rela->r_info);
      int rsym = ELF_REL_SYM(rela->r_info);
      struct Elf_Sym *sym = (struct Elf_Sym*) (dstart + symsect->sh_offset + (rsym * symbolsize));
      snprintf(buff, 1023, "Rela:: %p, (+%p), %p: %d, %d\n", rela->r_offset, rela->r_addend, (void*)rela->r_info,
                                                            rtype,
                                                            rsym);
      locked_print_string(buff, PRINTERR);

      const char * tp = "?";
      switch (rtype){
        default:
          break;
        case (ELF_RR_GLOBDAT):
          tp = "Glob";
       break;
        case (ELF_RR_RELATIVE):
          tp = "Rel";
          break;
        case (ELF_RR_JMPSLOT):
          tp = "Jmpslot";
       break;

      }
      newval = /*(*vicloc) +*/ rela->r_addend + adminstart->base /*+ sectloc*/ + sym->st_value;
      snprintf(buff, 1023, "Changing %s '%s' Size %d: %p <secl:%p, a:0x%x Symv:%p> @%p to %p\n",
          tp, elf_symname((Elf_Addr)dstart,   symsect,sym,ehdr),sym->st_size,*vicloc,
                                                                (void*)sectloc, rela->r_addend,sym->st_value, (void*)*vicloc, (void*)newval);
      *vicloc = newval;
      locked_print_string(buff, PRINTERR);

      r += s->sh_entsize;
    }
  }
  return 0;
}
 
int elf_spawn(char *dstart, size_t size, struct admin_s *adminstart,
              int verbose, enum e_settings flags){
  Elf_Addr base = adminstart->base;
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  if (verbose) {
    char buff[1024];
    snprintf(buff, 1023, "Spawning program from %p of size %p with flags %d\n", (void*)dstart, (void*) size, flags);
    locked_print_string(buff, PRINTERR);
  }

  function_spawn((main_function_t*) ((char*)base + ehdr->e_entry),
      adminstart);
  return 0;
}

static size_t strsize(char *in){
  size_t s = 0;
  while (in && in[s]){ s++; }
  return s + 1;//Null byte inc.
}

Elf_Addr argsize(int argc, char** argv,char * envp){
  int i=0;
  size_t si = sizeof(char*) * (1 + argc);
  //Argv
  for (i=0;i<argc;i++){
    si += strsize(argv[i]);
  }

  i = 0;
  while (envp && (envp[i] || envp[i+1])){
    i++;
    si++;
  }
  si += 2;

  return si;
}

int argsave(Elf_Addr start,struct admin_s *ab,int argc, char** argv,char * envp){
  int i=0;
  char **acpp = (char **)start;
  char *cpnt = (char*) (start + (sizeof(char*) *(argc + 1)));
  ab->argc = argc;
  ab->argv = acpp;
  //Argv
  for (i=0;i<argc;i++){
    int k;
    acpp[i] = cpnt;//Argv
    acpp[i+1] = NULL;
    *cpnt = 0;

    //Now copy the string
    k = 0;
    while (argv && argv[i] && (argv[i])[k]){
      *cpnt = (argv[i])[k];
      cpnt++;
      *cpnt = 0;
      k++;
    }
    cpnt++;//Room for nullbyte at arg end
  }
  cpnt++;

  i = 0;
  ab->envp = cpnt;
  while (envp && (envp[i] || envp[i+1])){
    cpnt[i] = envp[i];
    i++;
  }
  cpnt[i] = 0;
  cpnt[i+1] = 0;

  return 0;
}

// Load the program image into the memory,
int elf_loadprogram(char* data, size_t size, int verbose,
                    enum e_settings flags,
                    int argc, char **argv, char *envp
                    ){
  struct admin_s params;
  params.fname = NULL;
  params.base = 0;
  params.core_start = -1;
  params.core_size = -1;

  params.argc = argc;
  params.argv = argv;
  params.envp = envp;

  return elf_loadprogram_p(data, size, verbose, flags, &params);
}

int elf_loadprogram_p(char *data, size_t size, int verbose,
    enum e_settings flags, struct admin_s * params){
  Elf_Addr relbase;
  struct admin_s *p = NULL;
  locked_newbase(&p);
  
  if (elf_header_marshall(data,size)){
    locked_print_string("Marshalling failed\n", PRINTERR);
    return -1;
  }
  if (elf_header_check(data,size)){
    locked_print_string("Header checking failed\n", PRINTERR);
    return -1;
  }
  if (elf_header_check_arch(data,size)){
    locked_print_string("Architecture check failed\n", PRINTERR);
    return -1;
  }

  relbase = elf_findbase_marshallphdr(data, size);
  if (verbose) {
    char buff[1024];
    snprintf(buff, 1023, "base_file: %p\n", (void*)relbase);
    locked_print_string(buff, PRINTERR);
  }

  //Elf_Addr theargsize = argsize(p->argc, p->argv, p->envp);
  //Prepare storage for argv and env
  //IN A MAGIC ELF SECTION TODO
  //base = (Elf_Addr)reserve_range((void*)p->base, adminsize, perm_read|perm_write);
  

  p->fname = params->fname;
  p->core_start = params->core_start;
  p->core_size = params->core_size;

  //magic loading, until then:
  p->argc = 0;
  p->argv = NULL;
  p->envp = NULL;
  
  p->base += relbase;//correct for elf base
  //force Allignment
  static const int PAGE_SIZE = 4096;
  p->base = p->base & -PAGE_SIZE;
  
  if (verbose) {
    char buff[1024];
    snprintf(buff, 1023, "base_used: %p\n", (void*)p->base);
    locked_print_string(buff, PRINTERR);
  }

  /*
  if (argsave(((Elf_Addr)adminstart) + sizeof(struct admin_s),
              adminstart,
              p->argc,
              p->argv,
              p->envp)){
    locked_print_string("Problem in argument construction\n", PRINTERR);
  }*/

  if (elf_loadit(data, size, p, verbose)){
    locked_print_string("Elf loading failed\n", PRINTERR);
    return -1;
  }

  if (elf_sectionscan(data, size, p, verbose)){
    locked_print_string("Elf sections failed\n", PRINTERR);
    return -1;
  }
  
  if (elf_spawn(data, size, p, verbose, flags)){
    locked_print_string("Elf spawning failed\n", PRINTERR);
    return -1;
  }

  return  0;
}

