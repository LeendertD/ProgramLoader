/**
 * \file hworld.c
 * Prints a single string
 *
 **/


#include "../loadtheone/loader_api.h"

/** \brief prints 2 messages, returns 0.
 * \param argc nr of args, unused
 * \param argv arguments, unused
 * \param env Environment, unused
 * \param api API interface, used for print functions
 * */
int lmain(int argc, char **argv, char *env, struct loader_api_s *api){
  void (*output_string)(const char *, int) = api->print_string;
  void (*output_int)(int, int) = api->print_int;
  (void)api;
  (void)argc;
  (void)argv;
  (void)env;
  output_string("The answer is 42\n", 1);
  return 0; 
}
