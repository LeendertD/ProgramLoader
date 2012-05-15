#include "../loadtheone/loader_api.h"


/*Non static storage manages to CRASH*/
enum handled_by (*lbp)(int, const char*);

void bp(const char *msg){
  static int id = 0;
  (*lbp)(id, msg);
  id++;
}

const char *strs[] = {
"Hoi ",
"dit ",
"is ",

"een ",
"programma, ",
"of niet soms?\n",
0
};

const char * a = "123456";
int dudewheresmycard(void){
  return 43;
}
int iaintfoundit(void){
  int a;
  a=0;
  a=2;
  a=dudewheresmycard();
  return a;
}
int lmain(int argc, char **argv, char *env, struct loader_api_s *api){
  int (*s)(const char*,enum e_settings, int,char**,char*);
  void (*output_string)(const char *, int) = api->print_string;
  void (*output_int)(int, int) = api->print_int;
  void (*output_pointer)(void*, int) = api->print_pointer;

  output_pointer((char*)a, PRINTOUT);

  //Load the breakpoint function into a 'global' variable
  lbp = (void*)42;//api->breakpoint;
  int i;
  s = api->spawn;


  /*Use as argument, unrelated to argv...*/
  //bp("Pre print hw\n");
  output_string("hello world\n", PRINTOUT);
  //bp("Pre print ac\n");
  output_int(argc, PRINTOUT);
  //bp("Pre print av\n");
  output_int((int)(long)argv, PRINTOUT);
  //bp("Pre print env\n");
  output_int((int)(long)env, PRINTOUT);

  for (i=0;i<100000;i++) __asm__("nop");

  //bp("Pre print longthing\n");
  for (i=0;strs[i];i++){
    output_int(i, PRINTOUT);
    output_string("#\n'", PRINTOUT);
    output_pointer((void*)strs[i], PRINTOUT);
    output_string("\n'", PRINTOUT);
    output_string(strs[i], PRINTOUT);
    output_string("'...\n", PRINTOUT);
  }

  return 0; 
}
