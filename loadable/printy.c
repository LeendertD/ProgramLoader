/**
 * \file printy.c
 * Prints some things, and timing.
 *
 **/

#include "../loadtheone/loader_api.h"
#include <time.h>

/** 
 * \param argc nr of args, unused
 * \param argv arguments, unused
 * \param env Environment, unused
 * \param api API interface, used for print functions
 * \return zero
 * */

int lmain(int argc, char **argv, char*env, struct loader_api_s *api){
  void (*output_string)(const char *, int) = api->print_string;
  void (*output_int)(int, int) = api->print_int;
  int i;
  clock_t starttime;
  clock_t endtime;
  int seen = 0;
  (void)api;

  /** Notes the 'time' */
  starttime = clock();

  /** Prints who is running */
  output_string("Printy::\n",1);

  /** Print the number of arguments */
  output_string("Argc:",1);
  output_int(argc, 1);

  /** Print each argument */
  for (i=0;i<argc;i++){
    output_string("\n>'",1);
    output_string(argv[i],1);
    output_string("'=\n",1);
  }

  /** Print the entire environment */
  output_string("\n\nEnv\n",1);
  i = 0;
  seen = 0;
  while (env && (env[i] || env[i+1])){
    if (env[i]){
      if (!seen) {
        output_string("\nentry: ", 1);
        output_string(env + i,1);
        seen = 1;
      }
    } else {
      output_string("\n",1);
      seen = 0;
    }
    i++;
  }
  output_string("\n",1);

  endtime = clock();
  endtime -= starttime;

  /** Print elapsed 'time' */
  output_int(endtime, PRINTOUT);
  output_string("\n",1);
  return 0; 
}
