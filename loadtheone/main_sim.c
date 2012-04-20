#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*Call abort, we need a prototype*/
#include <svp/mgsim.h>
#include <stddef.h>
#include <svp/abort.h>

#include "ELF.h"
#include "loader.h"

#include <fcntl.h>
int main(int argc, char **argv){
  int i;

  init_admins();

  /*
  int ff = open("programs.cfg", O_RDONLY);
  if (ff != -1){
    if (! read(ff, NULL, 0)){
      elf_fromconf(ff);
    }
  }*/

  //Not interested in program name of 'this' program
  for (i=1;i < argc; i++){
    elf_fromconfname(argv[i]);
  }

  //Check whether any arguments where processed
  if (i==0){
    locked_print_string("No file to load\n", 2);
  }


  /*???  more loading, debug wait, ???*/

  locked_print_string("Returning from Loader main\n", 2);
  return 0;
}

