
#include "../loadtheone/loader_api.h"
#include <time.h>

//#include <svp/testoutput.h>


int lmain(int argc, char **argv, char*env, struct loader_api_s *api){
  //int (*s)(const char*,enum e_settings, int,char**,char*);
  void (*output_string)(const char *, int) = api->print_string;
  void (*output_int)(int, int) = api->print_int;
  int i;
  clock_t starttime;
  clock_t endtime;
  int seen = 0;
  //s = api->spawn;
  (void)api;

  starttime = clock();
  //for (seen=0;seen<10000;seen++) clock();
  output_string("Printy::\n",1);
  output_string("Argc:",1);
  output_int(argc, 1);
  for (i=0;i<argc;i++){
    output_string("\n>'",1);
    output_string(argv[i],1);
    output_string("'=\n",1);
  }
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

  output_int(endtime, PRINTOUT);
  output_string("\n",1);
  return 0; 
}
