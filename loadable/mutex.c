#include <svp/testoutput.h>


int a = 0;

sl_def(modify)
{
   a += 1;
}
sl_enddef

#define MAKE_CLUSTER_ADDR(First, Size) ((First)*2 + (Size))
sl_def(foo)
{
   sl_create(, MAKE_CLUSTER_ADDR(3, 1) ,,,,, sl__exclusive, modify);  
   sl_sync();
   output_int(a, 0);
}
sl_enddef

int main(void) {
   sl_create(,,,100,,,, foo);
   sl_sync();

   output_int(a, 0);
   return 0;
}

