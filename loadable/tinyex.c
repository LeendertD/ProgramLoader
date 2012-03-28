#include <svp/testoutput.h>
int test(void *p, int argc, char **argv, char*env){

  /*Use as argument, unrelated to argv...*/
  if (argc > 1) {
    output_string(argv[1],2);
    output_char('\n',2);
  }
  //output_string("hello world\n", 1);
  void (*s)(char*,int,char**,char*);
  s = p;
  /*Call 'me' with other args*/
  (*s)(argv[0], argc-1, argv, env);
  return 0;
}
