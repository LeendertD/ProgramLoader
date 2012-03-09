#include <svp/mgsim.h>
#include <stddef.h>
#include <assert.h>
#include <svp/abort.h>

void reserve(void *where, size_t sz_bits)
{
  assert(sz_bits >= 12 && sz_bits <= 19);
  mgsim_control(where, MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP, sz_bits - 12);
}

int main(void)
{
  reserve(0x1234500000, 17);  
  reserve(0x6789000000, 17);  
  svp_abort();
  return 0;
}
