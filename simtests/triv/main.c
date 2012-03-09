#include <stdio.h>

//Result:
//got 2429000 blocks
//before being killed
//UNUSED blocks
//
//Result:
//got 2429000 blocks
//Touched blocks
//
//Conclusion: keeps hogging until the address space is exhausted, killed unless
//ulimit is set.

int main(void){
  unsigned int blocks = 0;
  volatile int *pnt;
  while ((pnt = malloc(1024)) != NULL) {
    if (! (blocks %1000)) printf("got %u blocks\n", blocks);
      blocks++;
    int i;
    int pp = (int) pnt;
    for (i=0;i<(1024/sizeof(int));i++) pnt[i] = i + pnt;
  }
  free(pnt);

  printf("Got %u blocks\n", blocks);
  return 42;
}

