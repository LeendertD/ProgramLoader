/**
 * \file crt_fun.c
 * Runtime for loadable programs.
 *
 **/


extern int lmain(int argc, ...);

/** \brief Entry point.
 * \param argv array of command line arguments
 * \param argc the number of entries in argv
 * \param env pointer to nullbyte seperated strings, terminated by double
 * nullbyte
 * \param api pointer to api struct
 * \return passes on the return value of actual program
 * */
int _start(int argc, char**argv, char*env, void*api)
{
  return lmain(argc, argv, env, api);
}
