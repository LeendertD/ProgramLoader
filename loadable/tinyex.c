#include <svp/testoutput.h>

#include "../loadtheone/loader_api.h"

int test(int argc, char **argv, char *env, struct loader_api_s *api){
  int (*s)(const char*,enum e_settings, int,char**,char*);
  s = api->spawn;


  /*Use as argument, unrelated to argv...*/
  output_string("hello world\n", 1);
  output_int(argc, 1);
  output_int((long)argv, 1);
  output_int((long)env, 1);
  return 0; 
}
