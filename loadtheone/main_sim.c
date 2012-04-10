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

  int ff = open("programs.cfg", O_RDONLY);
  if (ff != -1){
    if (! read(ff, NULL, 0)){
      load_fromconf(ff);
    }
  }

  //Not interested in program name of 'this' program
  for (i=1;i < argc; i++){
    char *nargv[] = {(char*)"prog",(char*)NULL};
    char nenv[] = "a=b\0\0";
    elf_loadfile(argv[i], e_verbose,
        1, nargv, nenv);
  }

  //Check whether any arguments where processed
  if (i==0){
    locked_print_string("No file to load\n", 2);
  }


  /*???  more loading, debug wait, ???*/

  locked_print_string("Returning from Loader main\n", 2);
  return 0;
}

