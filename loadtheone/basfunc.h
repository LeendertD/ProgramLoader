/**
 *  Leendert van Duijn
 *  UvA
 *
 *  Functions used in support of loading programs.
 *
 *
 **/


#ifndef H_MEMFUNCS
#define H_MEMFUNCS

enum e_perms {
  perm_read = 1,
  perm_write = 2,
  perm_exec = 4,
  perm_none = 0
};

/** \return end of reserved range */
void* reserve_range(void *start, size_t size, enum e_perms);
//void patcher_zero(char *start, size_t size);
//void patcher_patch(char *start, size_t count, char *src, size_t len);
//

int streq(const char *a, const char *b);

#endif
