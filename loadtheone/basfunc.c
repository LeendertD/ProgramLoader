/**
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

#define PRINTCORE 2


//This is equal to 1 << bits ....?
size_t bytes_from_bits(size_t bits){
  size_t bs = 1;
  while (bits--) bs = bs << 1;
  return bs;
}

#define minpagebits ((size_t)  12)
#define maxpagebits ((size_t)  19)
static const size_t minpagebytes = (size_t)1 << minpagebits; 
static const size_t maxpagebytes = (size_t)1 << maxpagebits; 

int reserve_single(void *addr, size_t sz_bits){
  /**
   * Reserve a single page.
   * Returns an indication of success.
   * */
  if ((sz_bits >= minpagebits ) && (sz_bits <= maxpagebits)){
    mgsim_control(addr, MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP, sz_bits - minpagebits);
    return 0;
  }
  return -1;
}

void* reserve_range(void *addr, size_t bytes, enum e_perms perm){
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
    if (reserve_single(addr, sz_bits)) return 0;
    return addr + (1 << sz_bits);
  }
  pages = bytes / maxpagebytes;
  for (i=0;i<pages;i++){
    if (reserve_single(addr, maxpagebits)) return addr + resc;

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
    if (reserve_single(addr, sz_bits)) return 0;
    resc += 1 << sz_bits;
  }

  //What protection is wanted is told in perms, however, setting these...
  //mprotect(startaddr, bytes, PROT_NONE);
  return addr + resc;
}

/* These functions are abandoned for now....
void patcher_zero(char *start, size_t size){
  memset(start, 0, size);
}
void patcher_patch(char *start, size_t count, char *src, size_t len){
  while (count >0){
    fprintf(stderr, "PATCH %p,from %p, size %p\n",
        (void*)start,(void*)src,(void*)len);
    memcpy(start, src, len);
    start += len;
    count--;
  }
}

int fakemain(int arg, char **argv, char *anv){
  fprintf(stderr, "FAKEMAIN\n");
  return 42;
}
*/


/* This is the skeleton which boots a new program */
sl_def(thread_function,,
               sl_glparm(main_function_t* , f),
               sl_glparm(int, ac),
               sl_glparm(char**,  av),
               sl_glparm(char*, e)
    ){
  /* Boot a program, do SOME administration */
  main_function_t *f = sl_getp(f);
  int ac = sl_getp(ac);
  char **av = sl_getp(av);
  char *e = sl_getp(e);
  struct loader_api_s *p = &loader_api;
  
  char buff[1024];
  int exit_code = (*f)(ac, av, e, p);
  //int exit_code = fakemain(ac, av, e);
  
  if (exit_code != 0){
    /* Some feedback about termination */
    snprintf(buff, 1023, "program terminated with code %d\n", exit_code);
    locked_print_string(buff, PRINTERR);
  }

  snprintf(buff, 1023, "Program with main@%p terminated with %d\n", (void*)f, exit_code);
  locked_print_string(buff, PRINTERR);
}
sl_enddef

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


  /*Create, run PROFIT*/
  // optie 3 (asynchroon, nooit gesynchroniseerd)
  
  fprintf(stdout, "To cores: %d @ %d\n",params->core_size, params->core_start);
  
  if (params->core_start == -1){
    sl_create( , , , , , , , thread_function, 
      sl_glarg(main_function_t* ,, main_f),
      sl_glarg(int, , params->argc),
      sl_glarg(char**, , params->argv),
      sl_glarg(char*, , params->envp)
    );
    sl_detach();
  } else {
    int cad = MAKE_CLUSTER_ADDR(params->core_start,params->core_size);
    //What type exactly?
    sl_create( ,cad, , , , , , thread_function, 
      sl_glarg(main_function_t* ,, main_f),
      sl_glarg(int, , params->argc),
      sl_glarg(char**, , params->argv),
      sl_glarg(char*, , params->envp)
    );
    sl_detach();
  }

  return 0;
}


//This does not suply nice flags
int loader_spawn(const char *programma, int argc, char **argv, char *env) {
  return elf_loadfile(programma, 0, argc ,argv, env);
}

sl_def(slprintstr_fn,, sl_glparm(const char*, strp), sl_glparm(int, fd)){
  const char *val = sl_getp(strp);
  int fdd = sl_getp(fd);
  output_string(val, fdd);
}
sl_enddef

void locked_print_string(const char *stin, int fp){
  sl_create(, MAKE_CLUSTER_ADDR(PRINTCORE, 1) ,,,,, sl__exclusive, slprintstr_fn, sl_glarg(const char *, strp, stin), sl_glarg(int , fd, fp) );
  sl_sync();
}

sl_def(slprintint_fn,, sl_glparm(int, strp), sl_glparm(int, fd)){
  int val = sl_getp(strp);
  int fdd = sl_getp(fd);
  output_int(val, fdd);
}
sl_enddef

void locked_print_int(int val, int fp){
  sl_create(, MAKE_CLUSTER_ADDR(PRINTCORE, 1) ,,,,, sl__exclusive, slprintint_fn, sl_glarg(int, strp, val), sl_glarg(int , fd, fp) );
  sl_sync();
}

int streq(const char *a, const char *b){
  /* No dependency needed for simple comparison */
  int i=0;
  while (((a[i]) == (b[i])) && a[i]) i++;
  return (a[i]) == (b[i]);
}


