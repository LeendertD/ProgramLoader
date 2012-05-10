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
int main(int argc, char **argv){
  int i;

  init_admins();

  //Not interested in program name of 'this' program
  for (i=1;i < argc; i++){
    elf_fromconfname(argv[i]);
  }

  //Check whether any arguments where processed
  if (i==0){
    locked_print_string("No file to load\n", 2);
  }

  locked_print_string("Returning from Loader main\n", 2);
  return 0;
}

