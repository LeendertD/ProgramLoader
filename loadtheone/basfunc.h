/**
 * \file basfunc.h
 * \author Leendert van Duijn, UvA
 *
 *  \brief Functions used in support of loading programs.
 *
 *
 **/


#ifndef H_MEMFUNCS
#define H_MEMFUNCS

/**
 * \var e_perms
 *
 * perm_read, read allowed.
 * perm_exec, execution allowed.
 * perm_write, writing allowed.
 *
 * perm_none, nothing allowed.
 * \brief Used for memory access control.
 **/
enum e_perms {
  perm_read = 1,
  perm_write = 2,
  perm_exec = 4,
  perm_none = 0
};

/**
 * Allocate a range of memory, page based, allignment sensitive.
 * \param addr Pointer to the beginning of the memory range.
 * \param bytes The requested amount of memory.
 * \param perm Permissions for the reserved memory.
 * \param pid The owning PID.
 * \return end of reserved range.
 *
 * \brief Allocates a range of memory, owned by pid, permissions set to perm.
 * */
void* reserve_range(void *addr, size_t bytes, enum e_perms perm, long pid);
int reserve_cancel_pid(long pid);

//void patcher_zero(char *start, size_t size);
//void patcher_patch(char *start, size_t count, char *src, size_t len);
//

/**
 * \param a First string.
 * \param b Second string.
 * \return 0 if strings differ.
 * **/
int streq(const char *a, const char *b);

#endif
