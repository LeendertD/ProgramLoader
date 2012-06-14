
/** \file loader_api.h
 *
 * Loader API and supporting definitions and prototypes.
 * To be included by loaded programs for the API functions.
 **/

#ifndef H_LOADAPI_A
#define H_LOADAPI_A
#include <time.h>

/** Makes a numerical parameter for core placement */
#define MAKE_CLUSTER_ADDR(First, Size) ((First)*2 + (Size))

#ifndef PRINTERR
/** Parameter for locked_print, indicating stderr */
#define PRINTERR 2
#endif /* PRINTERR */

#ifndef PRINTOUT
/** Parameter for locked_print, indicating stdout */
#define PRINTOUT 1
#endif /* PRINTOUT */

#ifndef base_off
/** The address for address sub-space 'root' */
#define base_off (Elf_Addr)(1l << 50)
#endif /* base_off */

#ifndef base_progmaxsize
/** The size of an address sub-space **/
#define base_progmaxsize (Elf_Addr)(1l << 50)
#endif /* base_progmaxsize */

#ifndef ROOM_ENV
/** The identifier for env storage symbol **/
#define ROOM_ENV "__loader_room_env"
#endif /* ROOM_ENV */

#ifndef ROOM_ARGV
/** The identifier for argv storage symbol **/
#define ROOM_ARGV "__loader_room_argv"
#endif /* ROOM_ARGV */

/**
 * \param e_noprogname On true argv[0] is not set to the ELF filename.
 * \param e_timeit On (true && ENABLE_DEBUG && ENABLE_CLOCKCALLS) prints timing
 * information on proccess termination.
 * \param e_exclusive On true requests the MGSim for sl_exclusive on sl_create.
 **/
enum e_settings {
  e_noprogname = 1,
  e_timeit = 1 << 2,
  e_exclusive = 1 << 3
};

/**
 * Administrative structure for a process
 **/
struct admin_s {
  /** The pid for this entry */
  int pidnum;
  /** Numerical setting for verbosity */
  int verbose;
  /** e_settings for settings */
  unsigned long settings;
  /** The base address for this process */
  unsigned long base;
  /** The filename for the ELF file */
  char *fname;

  /** The starting core to load to, or -1 */
  int core_start;
  /** The number of allotted cores, or -1 */
  int core_size;
  /** Passed argc */
  int argc;
  /** Passed argv */
  char **argv;
  /** Passed env */
  char *envp;

  /** If set, set at PID allocation */
  clock_t createtick;

  /** If set, set at detach (control transfer) */
  clock_t detachtick;

  /** If set, set right after lmain returns */
  clock_t lasttick;

  /** If set, just before the PID is freed to the loader and printing of times */
  clock_t cleaneduptick;

  /** Function pointer to be called on timing event. */
  void (*timecallback)(void);

  /** Symbol information */
  unsigned long argroom_offset;
  /** Symbol information */
  unsigned long argroom_size;
  /** Symbol information */
  unsigned long envroom_offset;
  /** Symbol information */
  unsigned long envroom_size;

  /** Freelist 'pointer' */
  int nextfreepid;
};

/** Initializes an admin structure.
 * \param X Pointer to the structure to initialize,
 **/
#define ZERO_ADMINP(X)\
  (X)->timecallback = 0;\
  (X)->settings = 0;\
  (X)->pidnum = 0;\
  (X)->verbose = 0;\
  (X)->base = 0;\
  (X)->fname = 0;\
  (X)->core_start = 0;\
  (X)->core_size = 0;\
  (X)->argc = 0;\
  (X)->argv = 0;\
  (X)->envp = 0;\
  (X)->createtick = 0;\
  (X)->detachtick = 0;\
  (X)->cleaneduptick = 0;\
  (X)->argroom_offset = 0;\
  (X)->argroom_size = 0;\
  (X)->envroom_offset = 0;\
  (X)->envroom_size = 0;\
  (X)->nextfreepid = 0;

/**
 * Indicator who handled the exception, user,system,ignored (no).
 **/
enum handled_by {
  /**Not handled*/
  HANDLED_NO = 0,
  /**Handled by continue, run, step etc.*/
  HANDLED_USER,
  /**Configured to ignore 'this' breakpoint*/
  HANDLED_CONFIG 
};


/**
 *Structure passed as API
 * */
struct loader_api_s {
  /**Spawns a program from file, with parameters.**/
  int (*spawn)(const char *programma,
               enum e_settings,
               int argc, char **argv,
               char *env);

  /**Prints (blocking, locked)*/
  void (*print_string)(const char*, int);
  /**Prints (blocking, locked)*/
  void (*print_int)(int, int);
  /**Prints (blocking, locked)*/
  void (*print_pointer)(void*, int);
  /**Loads from config file*/
  void (*load_fromconf)(const char* fname);
  /**Loads from opened config file*/
  void (*load_fromconf_fd)(int fd);
  /**Loads from preconfigured structure*/
  int (*load_fromparam)(struct admin_s *, enum e_settings);
  /**Calls a system breakpoint*/
  enum handled_by (*breakpoint)(int id, const char *msg);
};


/** Central structure, passed to loaded programs */
extern struct loader_api_s loader_api;

#endif
