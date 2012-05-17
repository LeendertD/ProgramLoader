#include "../loadtheone/loader_api.h"

const char * msgcat(char *in){//Insecure easy string concat
  static char buff[4096];
  static const char * msg = "Starting:";
  int i=0;
  while (msg[i]){
    buff[i] = msg[i];
    i++;
  }
  while (in && (*in)){
    buff[i] = *in;
    i++;
    buff[i] = 0;
    in++;
  }
  return buff;
}

char **getargv(const char *in, int*c){
  static char* cps[128];
  static char buff[1024];
  int i=0;
  int a=0;
  char * old = buff;

  old=buff;
  for (i=0;in[i];i++){
    buff[i] = in[i];
    if (buff[i] == ' '){
      buff[i] = 0;
      cps[a] = old;
      a++;
      old = buff + i + 1;
    }
  }
  if (i){
    buff[i] = 0;
    cps[a] = old;
    a++;
  }
  *c = a;
  return cps;
}

int lmain(int argc, char **argv, char *env, struct loader_api_s *api){
  if (! (argc && argv && api)) return 0;
  struct admin_s cld;
  int i;
  void (*output_string)(const char *, int) = api->print_string;
  int (*s)(struct admin_s *, enum e_settings);
  s = api->load_fromparam;

  /*Use as argument, unrelated to argv...*/
  for (i=0; i<argc; i++){
    char **av = getargv(argv[i], &(cld.argc));
    output_string(msgcat(argv[i]), 1);

    /*Call 'things' with other args*/
    cld.verbose=0;
    cld.settings = e_timeit;
    cld.core_start = i;
    cld.core_size = 1;
    cld.argv = & av[1]; 
    cld.argc--;
    cld.fname = av[0];
    //Pass the env
    cld.envp = env;
    (*s)(&cld,0);
  }
  return 0; 
}
