#ifndef H_LOADAPI_A
#define H_LOADAPI_A


#define MAKE_CLUSTER_ADDR(First, Size) ((First)*2 + (Size))

enum e_settings {
  e_noprogname = 1,
  e_verbose    = 1 << 1
};

struct loader_api_s {
  int (*spawn)(const char *programma,
               enum e_settings,
               int argc, char **argv,
               char *env);

  void (*print_string)(const char*, int);
  void (*print_int)(int, int);
};



extern struct loader_api_s loader_api;

#endif
