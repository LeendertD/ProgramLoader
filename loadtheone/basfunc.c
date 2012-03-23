#include <svp/mgsim.h>
#include <stddef.h>
#include <svp/abort.h>
#include <assert.h>

//#include <sys/mman.h>

#include "basfunc.h"


size_t bytes_from_bits(size_t bits){
  size_t bs = 1;
  while (bits--) bs = bs << 1;
  return bs;
}

#define minpagebits ((size_t)  12)
#define maxpagebits ((size_t)  19)
/*..pagebits could not be static const, due to compiler errors in the bytes
 * calculation*/
static const size_t minpagebytes = (size_t)1 << minpagebits; 
static const size_t maxpagebytes = (size_t)1 << maxpagebits; 


size_t mem_bits_from_bytes(size_t bytes){
  size_t bits;
  assert(! (bytes % minpagebytes));
  bytes = bytes >> minpagebits;
  while (bytes){
    bytes = bytes > 1;
    bits++;
  }
  return bits + minpagebits;
}

int reserve_single(void *addr, size_t sz_bits){
  if ((sz_bits >= minpagebits ) && (sz_bits <= maxpagebits)){
    mgsim_control(addr, MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP, sz_bits - minpagebits);
    return MEM_OK;
  }
  return MEM_ERR_ARG;
}

int reserve_range(void *addr, size_t bytes, enum e_perms perm){
  size_t pages;
  void* startaddr = addr;
  size_t sz_bits = mem_bits_from_bytes(bytes);
  size_t i;

  if ((sz_bits >= minpagebits ) && (sz_bits <= maxpagebits)){
    return reserve_single(addr, sz_bits);
  }
  pages = bytes / maxpagebytes;
  for (i=0;i<pages;i++){
    int err = reserve_single(addr, maxpagebits);
    if (err != MEM_OK) return err;

    addr += maxpagebytes;
  }
  i = bytes % maxpagebytes;
  if (i) { //check off by one error?
    return reserve_single(addr, mem_bits_from_bytes(i));
  }

  //What protection is wanted is told in perms, however, setting these...
  //mprotect(startaddr, bytes, PROT_NONE);
  return MEM_OK;
}



int test_basfunc(void){//upper limit 2^63-1
  reserve_range((void*)0x9000000000000000,0x42000000, 0);
  svp_abort();
}
