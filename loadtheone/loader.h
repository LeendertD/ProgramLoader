#ifndef H_LOADY
#define H_LOADY
#include "ELF.h"
#include <svp/abort.h>
#define Verify(Expr, Message)                                  \
  if (!(Expr)) {                                               \
    fprintf(stderr, "Verification failure: %s\n", (Message) ); \
    svp_abort();                                                   \
  }

//
typedef int (main_function_t)(int argc, char **argv, char *envp);
void function_spawn(main_function_t * main_f);
int elf_loadprogram(char*, size_t, int, int fd, Elf_Addr base_sug);
#endif

