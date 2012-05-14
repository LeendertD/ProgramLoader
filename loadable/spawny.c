
#include "../loadtheone/loader_api.h"

int lmain(int argc, char **argv, char *env, struct loader_api_s *api){
  void (*output_string)(const char *, int) = api->print_string;
  //void (*output_int)(int, int) = api->print_int;
  int (*s)(const char*,enum e_settings, int,char**,char*);
  int val = *env - 'a';
  s = api->spawn;

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
    //output_int(val, 1);
    //output_char('\n', 1);
    (*env) += 1;
    /*Call 'me' with other args*/
    (*s)(argv[0], /*e_verbose*/0,  argc, argv, env);
    (*s)(argv[0], /*e_verbose*/0,  argc, argv, env);
  }
  return 0; 
}
