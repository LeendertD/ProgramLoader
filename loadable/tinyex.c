#include "../loadtheone/loader_api.h"

const char **strs = {
"Hoi ",
"dit ",
"is ",

"een ",
"programma, ",
"of niet soms?\n",
0
};

int test(int argc, char **argv, char *env, struct loader_api_s *api){
  int (*s)(const char*,enum e_settings, int,char**,char*);
  void (*output_string)(const char *, int) = api->print_string;
  void (*output_int)(int, int) = api->print_int;
  int i;
  s = api->spawn;


  /*Use as argument, unrelated to argv...*/
  output_string("hello world\n", PRINTOUT);
  output_int(argc, PRINTOUT);
  output_int((int)(long)argv, PRINTOUT);
  output_int((int)(long)env, PRINTOUT);


  for (i=0;strs[i];i++){
    output_int(i, PRINTOUT);
    output_string("\n", PRINTOUT);
    output_int((int)(long)strs[i], PRINTOUT);
    output_string("\n", PRINTOUT);
    output_string(strs[i], PRINTOUT);
    output_string("\n", PRINTOUT);
  }

  return 0; 
}
