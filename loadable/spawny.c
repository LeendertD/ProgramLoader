/**
 * \file spawnyo.c
 * Spawns some things, recursively, printing at each level
 *
 **/
#include "../loadtheone/loader_api.h"

/** \brief prints 2 messages, returns 0.
 * \param argc nr of args
 * \param argv arguments, uses first letter of argv[1] as a counter
 * \param env Environment, unused
 * \param api API interface, used for print,spawn functions
 * \return zero
 * */
int lmain(int argc, char **argv, char *env, struct loader_api_s *api){
  struct admin_s cld;
  
  //Emergency abort, no spawn function or args
  if (! (argc && argv && api)) return 0;
  if (argc != 2) return 0;

  void (*output_string)(const char *, int) = api->print_string;
  int (*s)(struct admin_s *, enum e_settings);

  //Find the 'counter' somethere
  int val = argv[1][0] - 'a';
  s = api->load_fromparam;

  const char *words[] = {
    "\nArg\n",
    "\nHow\n",
    "\nBe\n",
    "\nYe\n",
    "\nMatey\n"}; 

  //output_int(val,1);
  /*Use as argument, unrelated to argv...*/
  if (words[val]) {
    output_string(words[val], 1);

    argv[1][0]++;
    /*Call 'me' with other args*/
    cld.verbose=0;
    cld.fname = argv[0];
    cld.core_start = 0;
    cld.core_size = 0;
    cld.argc = argc;
    cld.argv = argv;
    cld.envp = env;
    (*s)(&cld,0);
    (*s)(&cld,0);
  }
  return 0; 
}
