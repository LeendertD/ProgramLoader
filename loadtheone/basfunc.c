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

#include "extrafuns.h"

/**
 * core used for print calls.
 * \brief A core used for blocking print requests
 * do not use this core for other threads
 */
#define PRINTCORE 2

/**
 * core used for race condition memory calls.
 * do not use this core for other threads
 */
#define MEMCORE 3


/**
 * \brief Size in bytes from address width.
 *  \param bits The address width.
 *  \return the size in bytes.
 */
size_t bytes_from_bits(size_t bits){
  return 1l << bits;
}


#ifndef minpagebits
/** Minimum bits page width*/
#define minpagebits ((size_t)  12)
#endif /*minpagebits*/

#ifndef maxpagebits
/** Maximum bits page width*/
#define maxpagebits ((size_t)  19)
#endif /*maxpagebits*/

/** Minimum number of bytes in a page */
static const size_t minpagebytes = (size_t)1 << minpagebits; 

/** Maximum number of bytes in a page */
static const size_t maxpagebytes = (size_t)1 << maxpagebits; 

/* \brief Do action param on a single page.
 * Reserve a single page.
 * Returns an indication of success.
 * \param addr The starting address.
 * \param sz_bits the page address width.
 * \param pid Which action to do MGSCTL_MEM_MAP for example.
 * \return 0 on success, -1 on invalid parameters
 * */
sl_def(lockme_reserve_single,, sl_glparm(void*, addr),sl_glparm(size_t, sz_bits), sl_glparm(long, pid) ){
  void *addr = sl_getp(addr);
  size_t sz_bits = sl_getp(sz_bits);
  long pid = sl_getp(pid);
  DOPID(pid);
  MAPONPID(addr, sz_bits-minpagebits);
}
sl_enddef

/** 
 * Allocate a single page.
 * \param addr Starting address of the page.
 * \param sz_bits The desired pade width.
 * \param pid The owning PID.
 * \return 0 on success
 **/
int reserve_single(void *addr, size_t sz_bits, long pid){

  if ((sz_bits >= minpagebits ) && (sz_bits <= maxpagebits)){
    sl_create(, MAKE_CLUSTER_ADDR(MEMCORE, 1) ,,,,, sl__exclusive, lockme_reserve_single, sl_glarg(void*, addr,addr),
                                                                                  sl_glarg(size_t,sz_bits,sz_bits),
                                                                                  sl_glarg(long, pid,pid) );
    sl_sync();
    return 0;
  }
  return -1;
}

/**
 * Cancel All owned (by pid) memory.
 * \param pid Which owning processes memory.
 * \return 0 on success.
 * */
int reserve_cancel_pid(long pid){
  UNMAPONPID(pid);
  return 0;
}

/** \brief Do action param on a range of memory.
 * \param addr the starting address.
 * \param bytes the requested size.
 * \param perm the requested permissions.
 * \param pid what pid it belongs to.
 * \return pointer to the end of the actual range.
 * The return value may be higher than addr+size due to page size limitations.
 */
void* reserve_range(void *addr, size_t bytes, enum e_perms perm, long pid){
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
    if (reserve_single(addr, sz_bits, pid)) return 0;
    return addr + (1 << sz_bits);
  }
  pages = bytes / maxpagebytes;
  for (i=0;i<pages;i++){
    if (reserve_single(addr, maxpagebits, pid)) return addr + resc;

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
    if (reserve_single(addr, sz_bits, pid)) return 0;
    resc += 1 << sz_bits;
  }

  /**
   * What protection is wanted is told in perms, however, setting these...
   * TODO
   * */
  //mprotect(startaddr, bytes, PROT_NONE);
  return addr + resc;
}

/* \brief This is the skeleton which boots a new program.
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
#endif /* ENABLE_CLOCKCALLS */
  
  if (exit_code != 0){
    /** \brief Could print some feedback about termination */
#if ENABLE_DEBUG
    if (params->verbose > VERB_ERR){
      char buff[1024];
      snprintf(buff, 1023, "program terminated with code %d\n", exit_code);
      locked_print_string(buff, PRINTERR);
    }
#endif /* ENABLE_DEBUG */
  }

#if ENABLE_DEBUG
  if (params->verbose > VERB_INFO){ 
      char buff[1024];
      snprintf(buff, 1023, "Program with main@%p terminated with %d\n", (void*)f, exit_code);
    locked_print_string(buff, PRINTERR);
  }
#endif /* ENABLE_DEBUG */
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
#endif /* ENABLE_DEBUG */
  //There are core placement specs
  int cad = MAKE_CLUSTER_ADDR(params->core_start,params->core_size);
  cad = (params->core_start == -1)?0:cad;

#if ENABLE_CLOCKCALLS
  params->detachtick = clock();
#endif /* ENABLE_CLOCKCALLS */

  if (params->settings & e_exclusive){ 
    sl_create( ,cad, , , , , sl__exlusive, thread_function, 
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

/*
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
 * \param stin the string.
 * \param fp the output stream PRINTERR or PRINTOUT
 * \return nothing
 */
void locked_print_string(const char *stin, int fp){
  sl_create(, MAKE_CLUSTER_ADDR(PRINTCORE, 1) ,,,,, sl__exclusive, slprintstr_fn, sl_glarg(const char *, strp, stin), sl_glarg(int , fd, fp) );
  sl_sync();
}

/**
 * \brief Prints a pointer (0x????????), blocking until done, no more that 1 at a time.
 * \param pl the printed value.
 * \param fp the output stream PRINTERR or PRINTOUT
 * \return nothing
 */
void locked_print_pointer(void* pl, int fp){
  char buff[128];
  buff[0] = 0;
  snprintf(buff,127, "%016lx", (unsigned long) pl);
  sl_create(, MAKE_CLUSTER_ADDR(PRINTCORE, 1) ,,,,, sl__exclusive, slprintstr_fn, sl_glarg(const char *, strp, buff), sl_glarg(int , fd, fp) );
  sl_sync();
}


/*
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
 * \param fp the output stream PRINTERR or PRINTOUT
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

