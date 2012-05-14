
#include "../loadtheone/loader_api.h"

int lmain(int argc, char **argv, char *env, struct loader_api_s *api){
  void (*output_string)(const char *, int) = api->print_string;
  void (*output_int)(int, int) = api->print_int;
  (void)api;
  (void)argc;
  (void)argv;
  (void)env;
  output_string("four two\n", 1);
  output_int(42, 1);
  return 0; 
}
