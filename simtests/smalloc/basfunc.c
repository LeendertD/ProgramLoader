#include <svp/mgsim.h>
#include <stddef.h>
#include <svp/abort.h>
#include <assert.h>

#define MEM_OK 0
#define MEM_ERR_ARG 1


int reserve_single(void *addr, size_t sz_bits){
  if ((sz_bits >= 12 ) && (sz_bits <= 19)){
    mgsim_control(addr, MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP, sz_bits - 12);
    return MEM_OK;
  }
  return MEM_ERR_ARG;
}

int reserve_range(void *addr, size_t sz_bits){
  size_t pages;
  if ((sz_bits >= 12 ) && (sz_bits <= 19)) return reserve_single(addr, sz_bits);



}

