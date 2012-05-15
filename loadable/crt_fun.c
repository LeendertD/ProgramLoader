
extern int lmain(int argc, ...);

int _start(int argc, char**argv, char*env, void*api)
{
  return lmain(argc, argv, env, api);
}
