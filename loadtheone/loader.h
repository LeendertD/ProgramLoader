#ifndef H_LOADY
#define H_LOADY
#include "ELF.h"
#include <svp/abort.h>
#define Verify(Expr, Message)                                  \
  if (!(Expr)) {                                               \
    fprintf(stderr, "Verification failure: %s\n", (Message) ); \
    svp_abort();                                                   \
  }

#include "loader_api.h"
//
typedef int (main_function_t)(int argc, char **argv, char *envp, void* spwn);


#define PRINTERR 2
#define PRINTOUT 1

void locked_print_int(int val, int fp);
void locked_print_string(const char*, int fp);

struct admin_s {
  Elf_Addr base;
  char *fname;
  int core_start;
  int core_size;
  int argc;
  char **argv;
  char *envp;
};

int function_spawn(main_function_t * main_f,
                    struct admin_s *);
 

int elf_loadprogram(char*, size_t, int verbose,
                    enum e_settings,
                    int argc, char **argv, char *env
    );
int elf_loadprogram_p(char*, size_t, int verbose,
                    enum e_settings,
                    struct admin_s *
    );


int elf_loadfile(const char *fname, enum e_settings flags,
              int argc, char **argv, char* env);
void elf_fromconfname(const char *fn);
void elf_fromconf(int fd);

int elf_loadfile_p(struct admin_s *, enum e_settings);

#endif

