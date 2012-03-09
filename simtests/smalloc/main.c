#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#define enabled 1
int printthings(void *n){
    if (! enabled) return 0;
    
    static void *lastbrk = NULL;
    void *nbreak = sbrk(0);
    int res;
    if (n){
        printf("%p\n", n);
    }
    if (nbreak != lastbrk) printf("New break of size 0x%x, %d\n", nbreak - lastbrk, nbreak - lastbrk);
    lastbrk = nbreak;
}
int main(void){

    void *r = NULL;
    int i;
    for (i=0;i<10000;i++){
        printthings(r);
        r = malloc(1024*10*20480);
    }
    printthings(r);
    return 0;
}
