/**
 * \file main_sim.c
 * \brief File housing bootstrap code.
 *  Leendert van Duijn
 *  UvA
 *
 *  Functions used in support of loading programs.
 *
 * Composit functions.
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <svp/mgsim.h>
#include <stddef.h>
#include <svp/abort.h>

#include "ELF.h"
#include "loader.h"

#include <fcntl.h>

/** \brief Main function, loads based on args.
 * \param argc Amount of arguments
 * \param argv Arguments: a.cfg b.cfg c.cfg...
 * \return 0 on success
 **/
int main(int argc, char **argv){
  int i;

  init_admins();

  /** Skip argv[0], Not interested in program name of 'this' program */
  for (i=1;i < argc; i++){
    elf_fromconfname(argv[i]);
  }

  /** Checks whether any arguments where processed, print a warning if none */
  if (i==0){
    locked_print_string("No file to load\n", 2);
  }

  /** Prints a friendly 'I'll be gone message */
  locked_print_string("Returning from Loader main\n", 2);
  return 0;
}

