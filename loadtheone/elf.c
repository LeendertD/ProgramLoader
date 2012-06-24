/**
 * \file elf.c
 * \brief File housing loader elf specific functions.
 *  Leendert van Duijn
 *  UvA
 *
 *  Functions used in support of loading programs.
 *
 * Basic and composite functions.
 *
 **/

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
#include <time.h>

/** Which node is used for PID/base allocation/determination */
#define NODE_BASELOCK 3

/** How much Process table entries to statically allocate */
#define MAXPROCS 1024

/** Indicator of incomplete ELF header, minimum size **/
#define SANE_SIZE sizeof(struct Elf_Ehdr)

/** Master process table, holds an entry for each running process */
struct admin_s proctable[MAXPROCS];

/** Index in the array/process table which should be free. Initially 1 */
int nextfreepid = 1;

/** Function to Initialize the process table, calling once should be quite enough. */
void init_admins(void){
  int i;
  for (i=0;i<MAXPROCS;i++){
    ZERO_ADMINP(&proctable[i]);
    proctable[i].nextfreepid = i+1;
  }
  proctable[MAXPROCS-1].nextfreepid = 0;
  nextfreepid = 1;
}

/* Pid/Base allocation code */
sl_def(slbase_fn,, sl_glparm(struct admin_s**, basep)){

  /* Sets the pointer to the allocated structure, handles the freelist */
  int npid = nextfreepid;
  struct admin_s **val = sl_getp(basep);
  *val = &proctable[npid];
  (*val)->base = base_off + npid * base_progmaxsize;
  (*val)->pidnum = npid;
  
  nextfreepid = (*val)->nextfreepid;
  (*val)->nextfreepid = 0;;
}
sl_enddef

/* Pid deallocation code */
sl_def(sldelbase_fn,, sl_glparm(int, deadpid)){

  /* Reclaims the PID for the system, handles the freelist */
  int npid = nextfreepid;
  int deadpid = sl_getp(deadpid);
  struct admin_s *val = &proctable[deadpid];
  val->pidnum = 0;
  
  val->nextfreepid = npid;
  nextfreepid = deadpid;
}
sl_enddef

/** \brief Generate a new base, PID actually.
 * \param params What structure pointer to update.
 * \return Base address.
 **/
Elf_Addr locked_newbase(struct admin_s **params){
  sl_create(, MAKE_CLUSTER_ADDR(NODE_BASELOCK, 1) ,,,,, sl__exclusive, slbase_fn, sl_glarg(struct admin_s**, params, params));
  sl_sync();

#if ENABLE_CLOCKCALLS
  /** As soon as possible without blocking others, notes the 'time' */
  (*params)->createtick = clock();
#endif /* clockcalls */
  return (*params)->base;
}

/** \brief Cleans a process.
 *  \param deadpid Which process to clean.
 **/
void locked_delbase(int deadpid){

#if ENABLE_CLOCKCALLS
  //Ready to DELETE the pid, last chance to acces data
  proctable[deadpid].cleaneduptick = clock();
  if (proctable[deadpid].timecallback) proctable[deadpid].timecallback();

#  if ENABLE_DEBUG
  /** If requested, prints timing data. */
  if ((proctable[deadpid].settings & e_timeit)/* || (proctable[deadpid].verbose > VERB_INFO)*/){
    char buff[1024];
    snprintf(buff, 1023, "\n<Clocks>%d,%d,%d,%lu,%lu,%lu,%lu</Clocks>%s\n",
        deadpid,
        proctable[deadpid].core_start,
        proctable[deadpid].core_size,

      proctable[deadpid].createtick,
      proctable[deadpid].detachtick - proctable[deadpid].createtick,
      proctable[deadpid].lasttick - proctable[deadpid].createtick,
      proctable[deadpid].cleaneduptick - proctable[deadpid].createtick,
      ""/*((proctable[deadpid].fname)?(proctable[deadpid].fname):"")*/
      );
    locked_print_string(buff, PRINTERR);
  }
#  endif /* ENABLE_DEBUG */
#endif /* ENABLE_CLOCKCALLS */


  /* DEALLOC MEMRANGES FIRST OR BOOM */
  reserve_cancel_pid(deadpid);
  sl_create(, MAKE_CLUSTER_ADDR(NODE_BASELOCK, 1) ,,,,, sl__exclusive, sldelbase_fn, sl_glarg(int, deadpid, deadpid));
  sl_detach();
}

/** \brief Loads a file from params.
 * \param params The prered settings.
 * \param flags Any flags required.
 * \return 0 on success.
 **/
int elf_loadfile_p(struct admin_s * params, enum e_settings flags){
  int fin = -1;
  size_t fsize = 0;
  struct stat fstatus;
  void *fdata = NULL;
  size_t r;
  size_t sr;
  off_t toread;
  int verbose = params->verbose;
  char buff[1024];

#if ENABLE_DEBUG
  if (verbose > VERB_INFO){
    snprintf(buff, 1023,"Loading %s\n", params->fname);
    locked_print_string(buff, PRINTERR);
  }
#endif /* ENABLE_DEBUG */

  fin = open(params->fname, O_RDONLY);
  if (-1 == fin){

#if ENABLE_DEBUG
    if(verbose > VERB_ERR){
      const char *err = strerror(errno);
      snprintf(buff, 1023, "File could not be opened: %s\n", err);
      locked_print_string(buff, PRINTERR);
    }
#endif /* ENABLE_DEBUG */

    return 0;
  }
  if (fstat(fin, &fstatus)) {

#if ENABLE_DEBUG
    if (verbose > VERB_ERR){
      const char *err = strerror(errno);
      snprintf(buff, 1023, "File not statted %s: %s\n", params->fname, err);
      locked_print_string(buff, PRINTERR);
    }
#endif /* ENABLE_DEBUG */

    return 0;
  }

  fsize = fstatus.st_size;
  if (fsize < SANE_SIZE){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("Filesize too small, not a valid file\n", PRINTERR);
#endif /* ENABLE_DEBUG */

    return 0;
  }

#if ENABLE_DEBUG
  if (verbose > VERB_INFO) locked_print_string("File opened\n", PRINTERR);
#endif /* ENABLE_DEBUG */

  /* Allocate temporary storage */
  fdata = malloc(fsize);
  sr = 0;
  toread = fsize;
  while (toread > 0){
    errno = 0;
    r = read(fin, fdata + sr, fsize);
    if (r != fsize){
      if (errno == 0){
        /* No error, just an incomplete read */
        toread -= r;
        sr += r;
        continue;
      }

#if ENABLE_DEBUG
      if (verbose > VERB_ERR){
        const char *err = strerror(errno);
        snprintf(buff, 1023, "Read error: %s, %d of %d read\n",
                         err, (int)r, (int)fsize);
        locked_print_string(buff, PRINTERR);
      }
#endif /* ENABLE_DEBUG */

      return 0;
    }
  }

#if ENABLE_DEBUG
  if (verbose> VERB_INFO) locked_print_string("File read\nClosing file\n", PRINTERR);
#endif /* ENABLE_DEBUG */

  close(fin);
  fin = -1;
  /*File closed*/

  if (elf_loadprogram_p(fdata, fsize,
        flags,
        params
        )){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("Elf failure\n", PRINTERR);
#endif /* ENABLE_DEBUG */

  }
  
  free(fdata);
  return 0;
}

/** \brief Loads from C like parameters.
 * \param fname What ELF file to load.
 * \param flags Settings for verbosity and such.
 * \param argc Number of entries in passed argv.
 * \param argv String pointer array.
 * \param env Environment, terminated by double nullbyte, strings seperated by
 * single nullbyte.
 * \return 0 on success.
 **/
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

/** \brief Byte order. 
 * \param dstart ELF data pointer.
 * \param size ELF image size.
 * \return 0 on success.
 **/
int elf_header_marshall(char *dstart, size_t size){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;

  if (size < sizeof(struct Elf_Ehdr)){

#if ENABLE_DEBUG
    locked_print_string("ELF file too short or truncated", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }
  /** Unmarshall header */
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

/** \brief Check header, print errors etc.
 * \param dstart ELF image pointer.
 * \param size ELF image size.
 * \param verbose Wheter to spam errors
 **/
int elf_header_check(char *dstart, size_t size, int verbose){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;


  /** Checks file header (signature) **/
  if ((dstart + size) < (char*)(& ehdr->e_ident[EI_MAG3])) {
#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("Header problem, size too low\n", PRINTERR);
#endif /* ENABLE_DEBUG */
    return -1;
  }

  /* Check file signature */
  if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
         ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
         ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
         ehdr->e_ident[EI_MAG3] == ELFMAG3){
    return 0;
  } else {

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("invalid ELF file signature", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }
}

/** \brief Checks architecture.
 * \param dstart ELF image pointer.
 * \param size ELF image size.
 * \param verbose Wheter to spam errors
 **/
int elf_header_check_arch(char *dstart, size_t size, int verbose){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;

  /** Checks that this file is for our 'architecture', ELF version */
  if (ehdr->e_ident[EI_VERSION] != EV_CURRENT){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR)    locked_print_string("ELF version mismatch", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }
  
  /** Checks bitsize **/
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR)    locked_print_string("file is not of proper bitsize", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }

  /** Byte order **/
  if (ehdr->e_ident[EI_DATA] != ELFDATA){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR)    locked_print_string("file is not of proper endianness", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }

  /** Checks machine type **/
  if (! (ehdr->e_machine == MACHINE_NORMAL ||
         ehdr->e_machine == MACHINE_LEGACY)){
#if ENABLE_DEBUG
    if (verbose > VERB_ERR)    locked_print_string("target architecture is not supported", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }

//#ifdef CHECK_EXECELF_ENABLETHISTOBREAKSTUFF
// if (ehdr->e_type != ET_EXEC){
//   locked_print_string("file is not an executable file", PRINTERR);
//   return -1;
// }
//#endif

  /** Checks existance of program header **/
  if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0){
#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("file has no program header", PRINTERR);
#endif /* ENABLE_DEBUG */
    return -1;
  }

  /** Checks program header size **/
  if (ehdr->e_phentsize != sizeof(struct Elf_Phdr)){
#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("file has an invalid program header", PRINTERR);
#endif /* ENABLE_DEBUG */
    return -1;
  }

  /** Checks ELF image size consistency with the program headers **/
  if (! (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize <= size)){
#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("file has an invalid program header", PRINTERR);
#endif /* ENABLE_DEBUG */
    return -1;
  }
  return 0;
}

/** \brief Header marshall.
 * \param phdr Elf header to fix.
 **/
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

/** \brief Finds base, and marshalls headers.
 * \param dstart ELF image pointer.
 * \param size ELF image size.
 * \return ELF assumed base
 **/
Elf_Addr elf_findbase_marshallphdr(char *dstart, size_t size){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  struct Elf_Phdr *phdr = (struct Elf_Phdr*) (dstart + ehdr->e_phoff);

  /* Size check not currently implemented */
  (void) size;


  /** Determine base address and check for loadable segments */
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
  return base;
}

/** \brief Loads from read ELF file.
 * \param dstart ELF image pointer.
 * \param size ELF image size.
 * \param adminstart Administration for the to be loaded process.
 * \return 0 on success.
 **/
int elf_loadit(char *dstart, size_t size, struct admin_s* adminstart){
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  struct Elf_Phdr *phdr = (struct Elf_Phdr*) (dstart + ehdr->e_phoff);
  Elf_Addr base = adminstart->base;
  int verbose = adminstart->verbose;
  Elf_Half i;
  long pid = adminstart->pidnum;
  char buff[1024];

#if ENABLE_DEBUG
  if (verbose > VERB_INFO) {
    char buff[1024];
    snprintf(buff, 1023,
        "Trying to load program: %p size %p @ %p\n",
        (void*)dstart, (void*)size, (void*)base);
    locked_print_string(buff, PRINTERR);
  }
#endif /* ENABLE_DEBUG */

  /* Then copy the LOAD segments into their right locations */
  for (i=0; i < ehdr->e_phnum; ++i){
    if (phdr[i].p_type == PT_LOAD && phdr[i].p_memsz > 0){
      if (phdr[i].p_memsz < phdr[i].p_filesz){

#if ENABLE_DEBUG
        if (verbose > VERB_ERR) {
          locked_print_string("file has an invalid segment, wont fit into memory", PRINTERR);
        }
#endif /* ENABLE_DEBUG */

        return -1;
      }
      int perm = 0;
      if (phdr[i].p_flags & PF_R) perm |= perm_read;
      //perm |= IMemory::PERM_READ|IMemory::PERM_DCA_READ;
      if (phdr[i].p_flags & PF_W) perm |= perm_write;
      //IMemory::PERM_WRITE|IMemory::PERM_DCA_WRITE;
      if (phdr[i].p_flags & PF_X) perm |= perm_exec;
      //IMemory::PERM_EXECUTE;
            
      char *act_addr = ((char*)base) + phdr[i].p_vaddr;

      if ((phdr[i].p_offset + phdr[i].p_filesz) > size){

#if ENABLE_DEBUG
        if (verbose > VERB_ERR) {
          locked_print_string("file has an invalid segment: data incomplete", PRINTERR);
        }
#endif /* ENABLE_DEBUG */

        return -1;
      }

#if ENABLE_DEBUG
      if (verbose >VERB_TRACE) {
        char buff[1024];
        snprintf(buff, 1023,
            "load : %d_%d: size %p,%p @ %p with %d\n",
            phdr[i].p_type, i, (void*)phdr[i].p_memsz,
            (void*)phdr[i].p_filesz, act_addr, perm);
        locked_print_string(buff, PRINTERR);
      }
#endif /* ENABLE_DEBUG */

      //reserve, prepare data
      reserve_range(act_addr, phdr[i].p_memsz, perm |
          perm_read|perm_write|perm_exec,
          pid
          );
      //Permissions are currently ALL, due to setting the contents, and the
      //backing code is not quite permission friendly yet...
      
      //If there is at least some data, copy it
      if (phdr[i].p_filesz){
        memcpy(act_addr, dstart + phdr[i].p_offset, phdr[i].p_filesz);
      }

      //If there is no data but room reserved (per spec: p_filesz < p_memsz
      //which is even valid for no file data at all)
      Elf_Addr deltasize = phdr[i].p_memsz - phdr[i].p_filesz;
      if (phdr[i].p_filesz < phdr[i].p_memsz){
        //beyond the supplied data, 0 as per spec
        memset(act_addr + phdr[i].p_filesz, 0, deltasize);
      }

#if ENABLE_DEBUG
      if (verbose > VERB_TRACE){
        char buff[1024];
        snprintf(buff, 1023,"Loader %p bytes loaded at %p, of which %p void\n",
              (void*) phdr[i].p_filesz,(void*) act_addr, (void*)deltasize);
        locked_print_string(buff, PRINTERR);
      }
#endif /* ENABLE_DEBUG */

    }
  }

#if ENABLE_DEBUG
  if (verbose > VERB_INFO){
    const char* type = (ehdr->e_machine == MACHINE_LEGACY)? "legacy":"microthreaded";
      
    snprintf(buff, 1023, "Loaded %s ELF binary with virtual base %p entry point %p\n",
        type, (void*)base, (void*)base + ehdr->e_entry);
    locked_print_string(buff, PRINTERR);
  }
#endif /* ENABLE_DEBUG */

  return 0;
}

/** \brief Translates identifier into human readable.
 * \param in The id to translate.
 * \return Static string.
 **/
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

/** \brief Retrieves symbol name.
 * \param base Memory base.
 * \param s Which section holds strings.
 * \param sym Which symbol.
 * \param ehdr ELF header.
 * \return string poitner.
 **/
const char * elf_symname(Elf_Addr base,struct Elf_Shdr* s, struct Elf_Sym* sym,struct Elf_Ehdr *ehdr){
  const char * stringdata = (const char*) ((struct Elf_Shdr*) (base + ehdr->e_shoff + (s->sh_link * ehdr->e_shentsize)))->sh_offset + base;
  return stringdata + sym->st_name;
}

/** \brief Section name.
 * \param base Memory base.
 * \param num Which section.
 * \param ehdr ELF header.
 * \return String pointer
 **/
const char *elf_sectname(Elf_Addr base, int num,struct Elf_Ehdr *ehdr){
  const char *stringdata = (const char *)((struct Elf_Shdr*) (base + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize)))->sh_offset + base;
  return(num + stringdata);
}

/** \brief Scan sections. 
 * \param dstart ELF image start.
 * \param size ELF image size.
 * \param adminstart Allotted administration block.
 * \param verbose Wheter to spam messages.
 * \return 0 on success.
 **/
int elf_sectionscan(char *dstart, size_t size, struct admin_s* adminstart, int verbose){
  (void)size;
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;
  //Elf_Addr base = adminstart->base;
  Elf_Half sectsize = ehdr->e_shentsize;
  Elf_Half numsects = ehdr->e_shnum;
  //Elf_Off sectoff = ehdr->e_shoff;
  Elf_Half strndx = ehdr->e_shstrndx;
  
  int relocs[1024] = {0};
  int nr_relocs = 0;
  int symtabind = 0;
  int symbolsize = 0;
  struct Elf_Sym*  symbase = NULL;
  struct Elf_Shdr*  symsect = NULL;
  Elf_Half i;
  char buff[1024];
  if (sectsize != sizeof(struct Elf_Shdr)){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("SIZE MISMATCH\n", PRINTERR);
#endif /* ENABLE_DEBUG */
  }

#if ENABLE_DEBUG
  if (verbose > VERB_TRACE){
    snprintf(buff,1023, "Got %d sections of size %d each Expecting size %lu\n"\
                      "Stringtable in section %d\n",
      numsects, sectsize, sizeof(struct Elf_Shdr), strndx);
    locked_print_string(buff, PRINTERR);

    snprintf(buff,1023, "\nSection %3s(%8s): %15s,%6s %14s,%14s,%14s,%14s, %6s,%6s, %14s,%14s\n", "#","Type",
        "Name",
        "Type#", "Flags", "Addr", "Off", "Size", "Link", "Info", "Align", "Entsize");
    locked_print_string(buff, PRINTERR);
  }
#endif /* ENABLE_DEBUG */

  for (i=0;i<numsects;i++){
    //Loop over the sections, check their need for further processing.
    struct Elf_Shdr *s = (struct Elf_Shdr*) (dstart + ehdr->e_shoff + (i*sectsize));

#if ENABLE_DEBUG
    if (verbose > VERB_TRACE){
      snprintf(buff,1023, "Section %3d(%8s): %15s,%6u, %14lu,%14lu,%14lu,%14lu, %6u,%6u, %14lu,%14lu\n", i,elf_sectiontype(s->sh_type),
          elf_sectname((Elf_Addr)dstart, s->sh_name, ehdr),
          s->sh_type, s->sh_flags, s->sh_addr, s->sh_offset, s->sh_size, s->sh_link, s->sh_info, s->sh_addralign, s->sh_entsize);
      locked_print_string(buff, PRINTERR);
    }
#endif /* ENABLE_DEBUG */

    switch (s->sh_type){
      case SECTION_DYNSYM:

#if ENABLE_DEBUG
        if (verbose > VERB_TRACE) locked_print_string("Setting (Dyn)symbol pointer\n", PRINTERR); 
        if (symtabind && (verbose > VERB_ERR)) locked_print_string("Error: Double symtab\n", PRINTERR);
#endif /* ENABLE_DEBUG */

        //Note the section, needed later
        symtabind = i;
        symbolsize = s->sh_entsize;
        symbase = (struct Elf_Sym*) (dstart + s->sh_offset);
        symsect = (struct Elf_Shdr*) s;

      case SECTION_SYMTAB:{
        unsigned  int r=0;
        int c = 0;
        if ((verbose > VERB_ERR) && (s->sh_entsize != sizeof(struct Elf_Sym))) locked_print_string("Size mismatch symtab\n", PRINTERR);
        while (r < s->sh_size){
          struct Elf_Sym *symt = (struct Elf_Sym*) (dstart + s->sh_offset + r);
          int bind = ELF_SYM_BIND(symt->st_info);
          int type = ELF_SYM_TYPE(symt->st_info);
          const char *name = elf_symname((Elf_Addr)dstart, s, symt,ehdr);
          if (streq(ROOM_ENV,name)){
            //Env room
            adminstart->envroom_offset = adminstart->base + symt->st_value;
            adminstart->envroom_size = symt->st_size;

#if ENABLE_DEBUG
            if (verbose > VERB_TRACE){
              snprintf(buff, 1023, "Envroom: %s@%p<%p>--> %p\n",name,(void*)symt->st_value, (void*)symt->st_size, (void*)adminstart->envroom_offset);
              locked_print_string(buff, PRINTERR);
            }
#endif /* ENABLE_DEBUG */

          } else if (streq(ROOM_ARGV,name)){
            //Env room
            adminstart->argroom_offset = adminstart->base + symt->st_value;
            adminstart->argroom_size = symt->st_size;

#if ENABLE_DEBUG
            if (verbose > VERB_TRACE){
              snprintf(buff, 1023, "Argroom: %s@%p<%p>--> %p\n",name,(void*)symt->st_value, (void*)symt->st_size, (void*)adminstart->argroom_offset);
              locked_print_string(buff, PRINTERR);
            }
#endif /* ENABLE_DEBUG */

          }

#if ENABLE_DEBUG
            if (verbose > VERB_TRACE){
            snprintf(buff, 1023, "Sym %3d: %15s(%d) %17p of size %5lu, %3d<%2d,%2d> %3d, %8d\n", c,
                name,
                symt->st_name,
                (void*)symt->st_value,
                symt->st_size,
                symt->st_info,
                bind,type,
                symt->st_other,
                symt->st_shndx
                );
            locked_print_string(buff, PRINTERR);
          }
#endif /* ENABLE_DEBUG */

          c++;
          r += s->sh_entsize;
        }
        break;
      }
      case SECTION_RELA:{
        //Note the needed relocs
        relocs[nr_relocs++] = i;

#if ENABLE_DEBUG
        if (verbose > VERB_TRACE) locked_print_string("Found RELA section\n", PRINTERR);
#endif /* ENABLE_DEBUG */

        break;}
      case SECTION_REL:{
        //Note the needed relocs
        relocs[nr_relocs++] = i;

#if ENABLE_DEBUG
        if (verbose > VERB_TRACE) locked_print_string("Found REL section\n", PRINTERR);
#endif /* ENABLE_DEBUG */

        break;}
      default:
        //Nothing of interest
        break;
    }
  }

  for (i=0;i<nr_relocs;i++){
    struct Elf_Shdr *s = (struct Elf_Shdr*) (dstart + ehdr->e_shoff + (relocs[i] * sectsize));
    unsigned int r=0;

#if ENABLE_DEBUG
    if ((s->sh_type == SECTION_RELA) &&
        (s->sh_entsize != sizeof(struct Elf_Rela))) locked_print_string("Size mismatch rel*\n", PRINTERR);
    if ((s->sh_type == SECTION_REL) &&
        (s->sh_entsize != sizeof(struct Elf_Rel))) locked_print_string("Size mismatch rel*\n", PRINTERR);
#endif /* ENABLE_DEBUG */

    while (r < s->sh_size){
      Elf_Addr off = 0;
      Elf_Sxword addend = 0;
      Elf_Addr info = 0;
      Elf_Addr * vicloc = 0;
      
      if (SECTION_RELA == s->sh_type){
        struct Elf_Rela *rela = (struct Elf_Rela*)(dstart + s->sh_offset + r);
        off = rela->r_offset;
        info = rela->r_info;
        //Determine the location of the relocating pointer
        vicloc = (Elf_Addr*) (adminstart->base + off);
        //The rela has an explicit addend
        addend = rela->r_addend;
      } else if (SECTION_REL == s->sh_type){
        struct Elf_Rel *rel = (struct Elf_Rel*)(dstart + s->sh_offset + r);
        off = rel->r_offset;
        info = rel->r_info;
        //Determine the location of the relocating pointer
        vicloc = (Elf_Addr*) (adminstart->base + off);
        //The rel has the addend on the targeted location
        addend = *vicloc;
      } else {
        continue;
      }
      int rtype = ELF_REL_TYPE(info);
      int rsym = ELF_REL_SYM(info);

      //The new pointee,calculated just below
      Elf_Addr newval = 0;

      struct Elf_Sym *sym = (struct Elf_Sym*) (dstart + symsect->sh_offset + (rsym * symbolsize));

#if ENABLE_DEBUG
      if (verbose > VERB_TRACE){
        snprintf(buff, 1023, "Rela:: %p, (+%p), %p: %d, %d\n", (void*)off, (void*)addend, (void*)info,
                 rtype, rsym);
        locked_print_string(buff, PRINTERR);
      }
#endif /* ENABLE_DEBUG */

      newval = addend + adminstart->base  + sym->st_value;

#if ENABLE_DEBUG
      if (verbose > VERB_TRACE){
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
         //This is used for printing the type, logic is the same
        }
        snprintf(buff, 1023, "Changing %s '%s' Size %lu: %p <a:0x%x Symv:%p> @%p to %p\n",
            tp, elf_symname((Elf_Addr)dstart,   symsect,sym,ehdr),sym->st_size,(void*)(*vicloc),
            (unsigned int)addend,(void*)sym->st_value, (void*)*vicloc, (void*)newval);

        locked_print_string(buff, PRINTERR);
      }
#endif /* ENABLE_DEBUG */

      *vicloc = newval;

      r += s->sh_entsize;
    }
  }
  return 0;
}
 
/** \brief Spawn a (loaded) program.
 * \param dstart Data start.
 * \param size Data size.
 * \param adminstart Where adminstration resides.
 * \param verbose Wheter to spam with errors.
 * \param flags Any required flags.
 * \return 0 on succes.
 **/
int elf_spawn(char *dstart, size_t size, struct admin_s *adminstart,
              int verbose, enum e_settings flags){
  Elf_Addr base = adminstart->base;
  struct Elf_Ehdr *ehdr = (struct Elf_Ehdr*)dstart;

#if ENABLE_DEBUG
  if (verbose > VERB_INFO) {
    char buff[1024];
    snprintf(buff, 1023, "Spawning program from %p of size %p with flags %d\n", (void*)dstart, (void*) size, flags);
    locked_print_string(buff, PRINTERR);
  }
#endif /* ENABLE_DEBUG */

  function_spawn((main_function_t*) ((char*)base + ehdr->e_entry),
      adminstart);
  return 0;
}

/** \brief String size, inlcudes \0
 * \param in Which string to size.
 * \return Size of a string, including null.
 **/
static size_t strsize(char *in){
  size_t s = 0;
  while (in && in[s]){ s++; }
  return s + 1;//Null byte inc.
}

/** \brief Checks the size of arguments and allotted storage.
 * \param argc As requested
 * \param argv As requested
 * \param envp As requested
 * \param asize Available argument space
 * \param esize Available environment space
 * \param verbose Wheter to spawn warnings
 * \return 0 on success, 1 on ArgumentsTooLarge, 2 on EnvironmentTooLarge, 3 on
 * BothTooLarge.
 **/
int argsizecheck(int argc, char** argv,char * envp, Elf_Addr asize, Elf_Addr esize, int verbose){
  int i=0;
  int rv = 0;
  size_t si = sizeof(char*) * (1 + argc);
  //Argv
  for (i=0;i<argc;i++){
    si += strsize(argv[i]);
  }
  if (si > asize){
    rv |= 1;
  }

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) {
      char buff[1024];
      snprintf(buff,1023, "Argroom needed %lu, %lu available\n",si, asize);
      locked_print_string(buff, PRINTERR);
    }
#endif /* ENABLE_DEBUG */

  si = 0;
  i = 0;
  while (envp && (envp[i] || envp[i+1])){
    i++;
    si++;
  }
  si += 2;


  if (si > esize){
    rv |= 2;
  }

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) {
      char buff[1024];
      snprintf(buff,1023, "Envroom needed %lu, %lu available\n",si, esize);
      locked_print_string(buff, PRINTERR);
    }
#endif /* ENABLE_DEBUG */

  return rv;
}

/** \brief Saves args and environment.
 * \param ab The process entry.
 * \param argc As requested.
 * \param argv As requested.
 * \param envp As requested.
 * \return 0 on success.
 **/
int argsave(struct admin_s *ab,int argc, char** argv,char * envp){
  //Argv
  if (ab->argroom_size && argv){
    int i=0;
    char **acpp = (char **)ab->argroom_offset;
    char *cpnt = (char*) (ab->argroom_offset + (sizeof(char*) *(argc + 1)));

    ab->argc = argc;
    ab->argv = acpp;
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
  }

  if (ab->envroom_size && envp){
    char *cpnt;
    int i = 0;
    ab->envp = (char*)ab->envroom_offset;
    cpnt = ab->envp;
    while (envp && (envp[i] || envp[i+1])){
      cpnt[i] = envp[i];
      i++;
    }
    cpnt[i] = 0;
    cpnt[i+1] = 0;
  }

  return 0;
}

/** \brief Load the program image into the memory and spawn.
 * \param data Data pointer.
 * \param size Data size.
 * \param verbose Wheter to spam with messages.
 * \param flags Any requested flags.
 * \param argc As requested.
 * \param argv As requested.
 * \param envp As requested.
 * \return 0 on success.
 **/
int elf_loadprogram(char* data, size_t size, int verbose,
                    enum e_settings flags,
                    int argc, char **argv, char *envp
                    ){
  struct admin_s params;
  params.fname = NULL;
  params.base = 0;
  params.core_start = -1;
  params.core_size = -1;
  params.verbose = verbose;

  params.argc = argc;
  params.argv = argv;
  params.envp = envp;

  return elf_loadprogram_p(data, size, flags, &params);
}

/** \brief Load the program from image and spawn.
 * \param data Data pointer.
 * \param size Data size.
 * \param flags Any requested flags.
 * \param params The prepared settings.
 * \return 0 on success.
 **/
int elf_loadprogram_p(char *data, size_t size,
    enum e_settings flags, struct admin_s * params){
  Elf_Addr relbase;
  struct admin_s *p = NULL;
  int verbose = params->verbose;
  locked_newbase(&p);
  
  if (elf_header_marshall(data,size)){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("Marshalling failed\n", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }
  if (elf_header_check(data,size, verbose)){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("Header checking failed\n", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }

  if (elf_header_check_arch(data,size, verbose)){

#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("Architecture check failed\n", PRINTERR);
#endif /* ENABLE_DEBUG */

    return -1;
  }

  relbase = elf_findbase_marshallphdr(data, size);

#if ENABLE_DEBUG
  if (verbose > VERB_TRACE) {
    char buff[1024];
    snprintf(buff, 1023, "base_file: %p\n", (void*)relbase);
    locked_print_string(buff, PRINTERR);
  }
#endif /* ENABLE_DEBUG */
  

  //Set transferablesettings
  p->fname = params->fname;
  p->core_start = params->core_start;
  p->core_size = params->core_size;
  p->verbose = params->verbose;
  p->settings = params->settings;

  p->base += relbase;//correct for elf base
  //force Allignment
  static const int PAGE_SIZE = 4096;
  p->base = p->base & -PAGE_SIZE;
  
#if ENABLE_DEBUG
  if (verbose > VERB_TRACE) {
    char buff[1024];
    snprintf(buff, 1023, "base_used: %p\n", (void*)p->base);
    locked_print_string(buff, PRINTERR);
  }
#endif /* ENABLE_DEBUG */
  
  if (elf_loadit(data, size, p)){
#if ENABLE_DEBUG
    if (verbose > VERB_ERR)  locked_print_string("Elf loading failed\n", PRINTERR);
#endif /* ENABLE_DEBUG */
    return -1;
  }

  if (elf_sectionscan(data, size, p, verbose)){
#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("Elf sections failed\n", PRINTERR);
#endif /* ENABLE_DEBUG */
    return -1;
  }


  //magic loading, fallback to NO ARGS:
  p->argc = 0;
  p->argv = NULL;
  p->envp = NULL;
  
  int sizechecks = argsizecheck(params->argc, params->argv, params->envp, p->argroom_size, p->envroom_size,  verbose);
  if (sizechecks & 1){//Argroom NO
#if ENABLE_DEBUG
    if (verbose > VERB_ERR) {
      locked_print_string("Arguments while not enough room available\n", PRINTERR);
    }
#endif /* ENABLE_DEBUG */
    params->argc = 0;
    params->argv = NULL;
  }
  if (sizechecks & 2){//Envroom NO
#if ENABLE_DEBUG
    if (verbose > VERB_ERR) {
      locked_print_string("Environment while not enough room available\n", PRINTERR);
    }
#endif /* ENABLE_DEBUG */
    params->argc = 0;
    params->argv = NULL;
  }
  if (params->argv || params->envp){
    if (argsave(p,
                params->argc,
                params->argv,
                params->envp)){
#if ENABLE_DEBUG
      if (verbose > VERB_ERR){
        locked_print_string("Problem in argument construction\n", PRINTERR);
      }
#endif /* ENABLE_DEBUG */
      
    }
  }
  
  if (elf_spawn(data, size, p, verbose, flags)){
#if ENABLE_DEBUG
    if (verbose > VERB_ERR) locked_print_string("Elf spawning failed\n", PRINTERR);
#endif /* ENABLE_DEBUG */
    return -1;
  }

  return  0;
}

