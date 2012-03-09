#include <stdio.h>

int main(void){
  char mid = 12;

  char *dah = &mid;
  while (dah++) printf("%x\n", (char)(*dah));
  return *dah;
}

