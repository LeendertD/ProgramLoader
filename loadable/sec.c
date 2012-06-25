/**
 * \file sec.c
 * Does a constant amount of nothing, then return
 *
 **/

#include "../loadtheone/loader_api.h"

/** \brief delays, returns 0.
 * \param argc nr of args, unused
 * \param argv arguments, unused
 * \param env Environment, unused
 * \param api API interface, unused
 * \return zero
 * */
int lmain(int argc, char **argv, char *env, struct loader_api_s *api){

  /** Counter, which should not be optimized */
  volatile long counter = 0;

  /** Counter target, which should not be optimized */
  volatile long end = 1024*64;

  /** Count, until done */
  while (counter != (end)) counter++;
  return 0; 
}
