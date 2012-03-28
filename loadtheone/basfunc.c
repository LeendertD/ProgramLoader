/**
 *  Leendert van Duijn
 *  UvA
 *
 *  Functions used in support of loading programs.
 *
 *
 **/




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
  //fprintf(stderr, "Reserving %d bits at %p\n", sz_bits, addr);
  if ((sz_bits >= minpagebits ) && (sz_bits <= maxpagebits)){
    mgsim_control(addr, MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP, sz_bits - minpagebits);
    return MEM_OK;
  }
  return MEM_ERR_ARG;
}

int reserve_range(void *addr, size_t bytes, enum e_perms perm){
  size_t pages;
  void* startaddr = addr;
  size_t sz_bits = 1;
  size_t i;
  size_t bbytes = bytes >> 1;
 
  while (bbytes > 0){
    bbytes = bbytes >> 1;
    sz_bits++;
  }

  if ((bytes >= minpagebytes ) && (bytes <= maxpagebytes)){
    return reserve_single(addr, sz_bits);
  }
  pages = bytes / maxpagebytes;
  for (i=0;i<pages;i++){
    int err = reserve_single(addr, maxpagebits);
    if (err != MEM_OK) return err;

    addr += maxpagebytes;
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
    return reserve_single(addr, sz_bits);
  }

  //What protection is wanted is told in perms, however, setting these...
  //mprotect(startaddr, bytes, PROT_NONE);
  return MEM_OK;
}


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

extern int spawn(const char *programma, int argc, char **argv, char *env) 
{
  // FIXME: implement a real spawn here.
  fprintf(stderr, "hello from spawn\n");
  return 0;
}


/* This is the skeleton which boots a new program */
sl_def(thread_function,,
               sl_glparm(main_function_t* , f),
               sl_glparm(int, ac),
               sl_glparm(char**,  av),
               sl_glparm(char*, e)
    ){
  main_function_t *f = sl_getp(f);
  int ac = sl_getp(ac);
  char **av = sl_getp(av);
  char *e = sl_getp(e);
  void *p = &spawn;
  int exit_code = (*f)(p, ac, av, e);

  //stdin,out, all others here?

  //int exit_code = fakemain(ac, av, e);
  if (exit_code != 0){
    /* Some feedback about termination */
    fprintf(stderr, "program terminated with code %d\n", exit_code);
  }

  fprintf(stderr, "Program with main@%p terminated with %d\n", (void*)f, exit_code);
}
sl_enddef

void function_spawn(main_function_t * main_f){

 
  // Placeholder for function argument determination
  char *argv[2] = {strdup("a.out"), strdup("HOI"), 0};
  int argc = 2;
  uint32_t env = 0x00000000;
  char *envp = (char*) (&env);
  

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
  sl_create( , , , , , , , thread_function, 
    sl_glarg(main_function_t* ,, main_f),
    sl_glarg(int, , argc),
    sl_glarg(char**, , argv),
    sl_glarg(char*, , envp)
  );
  sl_detach();
}


