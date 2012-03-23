#ifndef H_LOADY
#define H_LOADY

#include <svp/abort.h>
#define Verify(Expr, Message)                                  \
  if (!(Expr)) {                                               \
    fprintf(stderr, "Verification failure: %s\n", (Message) ); \
    svp_abort();                                                   \
  }



#endif

