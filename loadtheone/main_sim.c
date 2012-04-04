#include <stdlib.h>
#include <stdio.h>

/*Call abort, we need a prototype*/
#include <svp/mgsim.h>
#include <stddef.h>
#include <svp/abort.h>

#include "ELF.h"


#include "loader.h"


int main(int argc, char **argv){
  int i;


  //Not interested in program name of 'this' program
  for (i=1;i < argc; i++){
    char *nargv[] = {(char*)"prog",(char*)NULL};
    char nenv[] = "a=b\0\0";
    elf_loadfile(argv[i], e_verbose,
        1, nargv, nenv);
  }

  //Check whether any arguments where processed
  if (i==0){
    printf("No file to load\n");
  }


  /*???  more loading, debug wait, ???*/

  printf("Returning from Loader main\n");
  return 0;
}

