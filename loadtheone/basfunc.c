/**
 * \file basfunc.c
 * \brief File housing basic shared functions.
 *  Leendert van Duijn
 *  UvA
 *
 *  Functions used in support of loading programs.
 *
 * Basic or low level functions.
 *
 **/


#include <svp/testoutput.h>

#include <stdio.h>

#include <svp/mgsim.h>
#include <stddef.h>
#include <svp/abort.h>
#include <assert.h>

#include <string.h>
#include <stdint.h>
#include "basfunc.h"
#include "loader.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * \var PRINTCORE core used for print calls.
 * \brief A core used for blocking print requests
 * do not use this core for other threads
 */
#define PRINTCORE 2

/**
 * \brief Size in bytes from address width.
 *  \param bits The address width.
 *  \return the size in bytes.
 */
size_t bytes_from_bits(size_t bits){
  return 1l << bits;
}

#define minpagebits ((size_t)  12)
#define maxpagebits ((size_t)  19)
static const size_t minpagebytes = (size_t)1 << minpagebits; 
static const size_t maxpagebytes = (size_t)1 << maxpagebits; 

/** \brief Do action param on a single page.
 * Reserve a single page.
 * Returns an indication of success.
 * \param addr The starting address.
 * \param sz_bits the page address width.
 * \param param Which action to do MGSCTL_MEM_MAP for example.
 * \return 0 on success, -1 on invalid parameters
 * */
int reserve_param_single(void *addr, size_t sz_bits, int param){
  if ((sz_bits >= minpagebits ) && (sz_bits <= maxpagebits)){
    mgsim_control(addr, MGSCTL_TYPE_MEM, param , sz_bits - minpagebits);
    return 0;
  }
  return -1;
}

/** \brief Reserve a single page.
 * \param addr the starting address.
 * \param sz_bits the wanted page width
 * \return 0 on success.
 */
int reserve_single(void *addr, size_t sz_bits){
  return reserve_param_single(addr,sz_bits, MGSCTL_MEM_MAP);
}

/** \brief Remove a mapping for a single page
 * \param addr the starting address.
 * \param sz_bits the page width.
 * \return 0 on success.
 */
int reserve_cancel_single(void *addr, size_t sz_bits){
  return reserve_param_single(addr,sz_bits, MGSCTL_MEM_UNMAP);
}

/** \brief Do action param on a range of memory.
 * \param addr the starting address.
 * \param bytes the requested size.
 * \param perm the requested permissions.
 * \param param what action to do.
 * \return pointer to the end of the actual range.
 * The return value may be higher than addr+size due to page size limitations.
 */
void* reserve_param_range(void *addr, size_t bytes, enum e_perms perm,int param){
  /**
   * Reserve a range of bytes, try to obtain at least perm permissions.
   * Returns a pointer to the actual end of the range.
   * Which MIGHT be beyond the expeceted end, due to page size limitations.
   * */
  size_t pages;
  
  size_t sz_bits = 1;
  size_t i;
  size_t bbytes = bytes >> 1;
  size_t resc = 0;
  
  //No permissions available for now
  (void) perm;

  while (bbytes > 0){
    bbytes = bbytes >> 1;
    sz_bits++;
  }

  if ((bytes >= minpagebytes ) && (bytes <= maxpagebytes)){
    //If a single page is ok, use it.
    if (reserve_param_single(addr, sz_bits, param)) return 0;
    return addr + (1 << sz_bits);
  }
  pages = bytes / maxpagebytes;
  for (i=0;i<pages;i++){
    if (reserve_param_single(addr, maxpagebits, param)) return addr + resc;

    addr += maxpagebytes;
    resc += maxpagebytes;
  }
  i = bytes % maxpagebytes;
  if (i) { 
    sz_bits = 1;
    i = i >> 1;
    while (i > 0){
      i = i >> 1;
      sz_bits++;
    }
    if (sz_bits < minpagebits) sz_bits = minpagebits;
    if (reserve_param_single(addr, sz_bits, param)) return 0;
    resc += 1 << sz_bits;
  }

  /**What protection is wanted is told in perms, however, setting these... TODO*/
  //mprotect(startaddr, bytes, PROT_NONE);
  return addr + resc;
}

/** \brief Reserve a range of memory.
 * \param addr the starting address.
 * \param bytes the requested size.
 * \param perm the requested permissions.
 * \return pointer to the end of the actual range.
 *
 * The return value may be higher than addr+size due to page size limitations.
 */
void* reserve_range(void *addr, size_t bytes, enum e_perms perm){
  return reserve_param_range(addr,bytes, perm, MGSCTL_MEM_MAP);
}

/** \brief Reserve a range of memory.
 * \param addr the starting address.
 * \param bytes the requested size.
 * \param e_perms the requested permissions.
 * \return pointer to the end of the actual range.
 * The return value may be higher than addr+size due to page size limitations.
 */
void* reserve_cancel_range(void *addr, size_t bytes, enum e_perms perm){
  return reserve_param_range(addr,bytes, perm, MGSCTL_MEM_UNMAP);
}



/** \brief This is the skeleton which boots a new program.
 *  \param f the called main function.
 *  \param params the administration block used for settings and such.
 *  \return nothing.
 */
sl_def(thread_function,,
               sl_glparm(main_function_t* , f),
               sl_glparm(struct admin_s *, params)
    ){
  /* Boot a program, do SOME administration */
  main_function_t *f = sl_getp(f);
  struct admin_s *params = sl_getp(params);
  int ac = params->argc;
  char **av = params->argv;
  char *e = params->envp;
  struct loader_api_s *p = &loader_api;
  
  int exit_code = (*f)(ac, av, e, p);

#if ENABLE_CLOCKCALLS
  params->lasttick = clock();
#endif
  
  if (exit_code != 0){
    /** \brief Could print some feedback about termination */
#if ENABLE_DEBUG
    if (params->verbose > VERB_ERR){
      char buff[1024];
      snprintf(buff, 1023, "program terminated with code %d\n", exit_code);
      locked_print_string(buff, PRINTERR);
    }
#endif
  }

#if ENABLE_DEBUG
  if (params->verbose > VERB_INFO){ 
      char buff[1024];
      snprintf(buff, 1023, "Program with main@%p terminated with %d\n", (void*)f, exit_code);
    locked_print_string(buff, PRINTERR);
  }
#endif
  locked_delbase(params->pidnum);
}
sl_enddef

/**
 * \brief Spawns so that main_f gets called and cleaned up afterwards.
 * \param main_f The called function.
 * \param params The programs administrative data.
 * \return 0 on success.
 */
int function_spawn(main_function_t * main_f,
                    struct admin_s *params){
  // Placeholder for function argument determination
  //char *argv[] = {strdup("a.out"), strdup("HOI"), 0};
  //int argc = 2;
  //uint32_t env = 0x00000000;
  //char *envp = (char*) (&env);
  

  // optie 1 (synchroon):
  //sl_create( , , , , , , , thread_function, 
  //         sl_glarg(main_function_t* ,, main_f),
  //         sl_glarg(int, , argc),
  //         sl_glarg(char**, , argv),
  //         sl_glarg(char*, , envp)
  //  );
  //sl_sync();
  // optie 2 (asynchroon, met plan om later te synchroneseren):
  //sl_spawndecl(f);
  //sl_spawn(f, , , , , , , thread_function,
  //           sl_glarg(main_function_t* ,, main_f),
  //           sl_glarg(int, , argc),
  //           sl_glarg(char**, , argv),
  //           sl_glarg(char*, , envp)); 
  // dan elders: 
  //sl_spawnsync(f);


  /** \brief Create, run PROFIT*/
  // optie 3 (asynchroon, nooit gesynchroniseerd)
  
  char buff[1024];
  buff[0] = 0;
#if ENABLE_DEBUG
  if (params->verbose > VERB_INFO){
    snprintf(buff, 1023,
      "To cores: %d @ %d\n",params->core_size, params->core_start);
    locked_print_string(buff, PRINTERR);
  }
#endif
  //There are core placement specs
  int cad = MAKE_CLUSTER_ADDR(params->core_start,params->core_size);

  //Could be moved beyond the create, but the thread MIGHT be running already
  //need to check the docs
#if ENABLE_CLOCKCALLS
  params->detachtick = clock();
#endif
  if (params->core_start == -1){
    //Autocore, or actually just 'here'
    sl_create( , , , , , , , thread_function, 
      sl_glarg(main_function_t* ,, main_f),
      sl_glarg(struct admin_s*, , params)
    );
    sl_detach();
  } else {

    sl_create( ,cad, , , , , , thread_function, 
      sl_glarg(main_function_t* ,, main_f),
      sl_glarg(struct admin_s*, , params)
    );
    sl_detach();
  }

  return 0;
}


/**
 * \brief spawn a program based on filename and arguments.
 * \param programma the elf file.
 * \param argc passed argc
 * \param argv an string pointer array, strings get copied.
 * \param env double null terminated string table, gets copied.
 * \return 0 on success.
 */
int loader_spawn(const char *programma, int argc, char **argv, char *env) {
  return elf_loadfile(programma, 0, argc ,argv, env);
}

/**
 * \brief Prints a string, blocking until done, no more that 1 at a time.
 * \param strp the string.
 * \param fd the output stream PRINTERR or PRINTOUT
 * \return nothing
 */
sl_def(slprintstr_fn,, sl_glparm(const char*, strp), sl_glparm(int, fd)){
  const char *val = sl_getp(strp);
  int fdd = sl_getp(fd);
  output_string(val, fdd);
}
sl_enddef

/**
 * \brief Prints a string, blocking until done, no more that 1 at a time.
 * \param strp the string.
 * \param fd the output stream PRINTERR or PRINTOUT
 * \return nothing
 */
void locked_print_string(const char *stin, int fp){
  sl_create(, MAKE_CLUSTER_ADDR(PRINTCORE, 1) ,,,,, sl__exclusive, slprintstr_fn, sl_glarg(const char *, strp, stin), sl_glarg(int , fd, fp) );
  sl_sync();
}

/**
 * \brief Prints a pointer (0x????????), blocking until done, no more that 1 at a time.
 * \param pl the printed value.
 * \param fd the output stream PRINTERR or PRINTOUT
 * \return nothing
 */
void locked_print_pointer(void* pl, int fp){
  char buff[128];
  buff[0] = 0;
  snprintf(buff,127, "%016lx", (unsigned long) pl);
  sl_create(, MAKE_CLUSTER_ADDR(PRINTCORE, 1) ,,,,, sl__exclusive, slprintstr_fn, sl_glarg(const char *, strp, buff), sl_glarg(int , fd, fp) );
  sl_sync();
}


/**
 * \brief Prints a number, blocking until done, no more that 1 at a time.
 * \param pl the printed value.
 * \param fd the output stream PRINTERR or PRINTOUT
 * \return nothing
 */
sl_def(slprintint_fn,, sl_glparm(int, pl), sl_glparm(int, fd)){
  int val = sl_getp(pl);
  int fdd = sl_getp(fd);
  output_int(val, fdd);
}
sl_enddef

/**
 * \brief Prints a number, blocking until done, no more that 1 at a time.
 * \param val the printed value.
 * \param fd the output stream PRINTERR or PRINTOUT
 * \return nothing
 */
void locked_print_int(int val, int fp){
  sl_create(, MAKE_CLUSTER_ADDR(PRINTCORE, 1) ,,,,, sl__exclusive, slprintint_fn, sl_glarg(int, pl, val), sl_glarg(int , fd, fp) );
  sl_sync();
}

/**
 * \brief Checks equality of strings.
 * \param a string 'a'
 * \param b string 'b'
 * \return boolean true on |a| == |b| for N in 0:|a| && a[N] == b[N]
 */
int streq(const char *a, const char *b){
  /* No dependency needed for simple comparison */
  int i=0;
  while (((a[i]) == (b[i])) && a[i]) i++;
  return (a[i]) == (b[i]);
}


