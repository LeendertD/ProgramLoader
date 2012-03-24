#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/*Call abort, we need a prototype*/
#include <svp/mgsim.h>
#include <stddef.h>
#include <svp/abort.h>

#include "ELF.h"

#include <errno.h>

#include "loader.h"

#define SANE_SIZE sizeof(struct Elf_Ehdr)

int main(int argc, char **argv){
  char *fname = NULL;
  int fin = -1;
  off_t fsize = 0;
  char *data = NULL;
  struct stat fstatus;
  void *fdata = NULL;
  size_t r;
  size_t sr;
  off_t toread;
  int err;

  if (sizeof(ssize_t) != sizeof(off_t)){
    fprintf(stderr, "Warning, size missmatch %d,%d\n",
                     sizeof(ssize_t), sizeof(off_t));
  }

  if (argc == 2){
    fname = argv[1];
  } else {
    printf("No file to load\n");
    return 0;
  }

  printf("Loading %s\n", fname);
  fin = open(fname, O_RDONLY);

  if (-1 == fin){
    perror("File could not be opened");
    return 0;
  }
  if (fstat(fin, &fstatus)) {
    perror("File stat failed");
  }
  fsize = fstatus.st_size;
  if (fsize < SANE_SIZE){
    fprintf(stderr, "Filesize too small, not a valid file\n");
    return 0;
  }
  printf("File opened\n");
  fdata = malloc(fsize);


  //current offset
  sr = 0;
  toread = fsize;
  while (toread > 0){
    errno = 0;
    //read, hoping the offset gets incemented
    r = read(fin, fdata + sr, fsize);
    err = errno;
    if (r != fsize){
      if (err == 0){//No error, incomplete read
        //Interrupts are fine, as long as progress could be made
        toread -= r;
        sr += r;
        continue;
      }
      perror("Read error");
      fprintf(stderr, "Read error: %d, %d of %d read\n",
                       err, r, fsize);
      return 0;
    }
  }
  printf("File read\n");

  /*Data done, close file*/
  printf("Closing file\n");
  close(fin);
  fin = -1;
  /*File closed*/

  if (elf_loadprogram(fdata, fsize, 1)){
    fprintf(stderr, "Elf failure\n");
  }

  /*???*/


  
  printf("Returning from Loader main\n");
  return 0;
}

