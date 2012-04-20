#ifndef H_LOADAPI_A
#define H_LOADAPI_A


#define MAKE_CLUSTER_ADDR(First, Size) ((First)*2 + (Size))

#define PRINTERR 2
#define PRINTOUT 1
#define base_off (Elf_Addr)0x0010000000000000
enum e_settings {
  e_noprogname = 1,
  e_verbose    = 1 << 1
};

struct admin_s {
  int pidnum;
  //Elf_Addr base;
  long base;
  char *fname;
  int core_start;
  int core_size;
  int argc;
  char **argv;
  char *envp;

  int nextfreepid;
};



struct loader_api_s {
  int (*spawn)(const char *programma,
               enum e_settings,
               int argc, char **argv,
               char *env);

  void (*print_string)(const char*, int);
  void (*print_int)(int, int);
  void (*load_fromconf)(const char* fname);
  void (*load_fromconf_fd)(int fd);
  int (*load_fromparam)(struct admin_s *, enum e_settings);
 
};



extern struct loader_api_s loader_api;

#endif
