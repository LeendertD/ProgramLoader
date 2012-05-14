#ifndef H_LOADAPI_A
#define H_LOADAPI_A
#include <time.h>

#define MAKE_CLUSTER_ADDR(First, Size) ((First)*2 + (Size))

#define PRINTERR 2
#define PRINTOUT 1
#define base_off (Elf_Addr)0x0010000000000000


#define ROOM_ENV "room_env"
#define ROOM_ARGV "room_argv"

enum e_settings {
  e_noprogname = 1
};

struct admin_s {
  int pidnum;
  int verbose;
  //Elf_Addr base;
  unsigned long base;
  char *fname;
  int core_start;
  int core_size;
  int argc;
  char **argv;
  char *envp;

  clock_t createtick;
  clock_t detachtick;
  clock_t lasttick;
  clock_t cleaneduptick;


  unsigned long argroom_offset;
  unsigned long argroom_size;
  unsigned long envroom_offset;
  unsigned long envroom_size;

  int nextfreepid;
};

#define ZERO_ADMINP(x)\
  (x)->pidnum = 0;\
  (x)->verbose = 0;\
  (x)->base = 0;\
  (x)->fname = 0;\
  (x)->core_start = 0;\
  (x)->core_size = 0;\
  (x)->argc = 0;\
  (x)->argv = 0;\
  (x)->envp = 0;\
  (x)->createtick = 0;\
  (x)->detachtick = 0;\
  (x)->cleaneduptick = 0;\
  (x)->argroom_offset = 0;\
  (x)->argroom_size = 0;\
  (x)->envroom_offset = 0;\
  (x)->envroom_size = 0;\
  (x)->nextfreepid = 0;

enum handled_by {
  HANDLED_NO = 0,
  HANDLED_USER,
  HANDLED_CONFIG 
};


struct loader_api_s {
  int (*spawn)(const char *programma,
               enum e_settings,
               int argc, char **argv,
               char *env);

  void (*print_string)(const char*, int);
  void (*print_int)(int, int);
  void (*print_pointer)(void*, int);
  void (*load_fromconf)(const char* fname);
  void (*load_fromconf_fd)(int fd);
  int (*load_fromparam)(struct admin_s *, enum e_settings);
  enum handled_by (*breakpoint)(int id, const char *msg);
 
};



extern struct loader_api_s loader_api;

#endif
