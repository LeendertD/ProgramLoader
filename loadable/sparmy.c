/**
 * \file sparmy.c
 * Spawns all arguments.
 *
 **/
#include "../loadtheone/loader_api.h"

/** \brief Insecure concat.
 * \param in the string to append to 'starting:'
 * \return pointer to static buffer
 * */
const char * msgcat(char *in){//Insecure easy string concat
  /** Buffer, determines the maximum size of resulting string */
  static char buff[4096];

  static const char * msg = "Starting:";
  int i=0;

  /* Copies the initial string */
  while (msg[i]){
    buff[i] = msg[i];
    i++;
  }

  /* Copies the payload */
  while (in && (*in)){
    buff[i] = *in;
    i++;
    buff[i] = 0;
    in++;
  }
  return buff;
}
   
/** \brief prints 2 messages, returns 0.
 * \param argc nr of args
 * \param argv arguments, a list of ELF filenames
 * \param env Environment, unused
 * \param api API interface, used for print,spawn functions
 * \return zero
 * */
int lmain(int argc, char **argv, char *env, struct loader_api_s *api){
  if (! (argc && argv && api)) return 0;
  struct admin_s cld;

  /*clients arugments array*/
  char *runargv[] = {""};
  
  int i;
  void (*output_string)(const char *, int) = api->print_string;

  int (*s)(struct admin_s *, enum e_settings);
  /* The spawn function */
  s = api->load_fromparam;

  /*Call */
  cld.verbose=0;
  cld.settings = 0;
  cld.core_start = 64;
  cld.core_size = 1;
  cld.argv = runargv; 
  cld.argc = 0;
  cld.fname = argv[1];
  //Pass the env
  cld.envp = env;
  (*s)(&cld,0);
  /*Use as argument, unrelated to argv...*/
  for (i=1; i<argc; i++){

    /*Call 'things' with args as specced */
    cld.verbose=0;
    cld.settings = e_timeit;
    cld.core_start = 64 + (i%64);
    cld.core_size = 1;
    cld.argv = runargv; 
    cld.argc = 0;
    cld.fname = argv[i];
    //Pass the env
    cld.envp = env;
    /* Make the spawn call */
    (*s)(&cld,0);
  }
  return 0; 
}

