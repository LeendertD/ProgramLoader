#ifndef H_XDNO
#define H_XDNO

/**
 * \file extrafuns.h
 *
 * \brief Defines for platform critical functions/constructs.
 **/

#include <svp/mgsim.h>

#ifndef MGSCTL_MEM_MAP_WITH_PID
/**
 * Parameter for Control signal.
 * \brief Map with PID owning the memory.
 **/
#define MGSCTL_MEM_MAP_WITH_PID 2
#endif
#ifndef MGSCTL_MEM_EXTRA 
/**
 * Parameter for Control signal.
 * \brief PID related control.
 **/
#define MGSCTL_MEM_EXTRA 3
#endif
#ifndef MGSCTL_MEM_EXTRA_SETPID
/**
 * Parameter for Control signal.
 * \brief PID related control.
 **/
#define MGSCTL_MEM_EXTRA_SETPID 0
#endif
#ifndef MGSCTL_MEM_EXTRA_UNMAP_BY_PID
/**
 * Parameter for Control signal.
 * \brief PID related control, deallocation by owner.
 **/
#define MGSCTL_MEM_EXTRA_UNMAP_BY_PID 1
#endif

/**
 * Signal a breakpoint, which in interactive simulator mode might be continued
 * from.
 * In non-interactive mode it likely terminates the system.
 * \brief Signal a systemwide breakpoint.
 *
 * */
#define STEP() mgsim_control(0, MGSCTL_TYPE_STATACTION, MGSCTL_SA_EXCEPTION, 0)

/**
 * \param Addr The starting address.
 * \param Pagewidth Bit width of the allocated page.
 * As if calling mmap(addr, size, pid = 0)
 * \brief Allocates a single page.
 **/
#define MAPNOPID(Addr, Pagewidth) \
(mgsim_control((Addr), MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP, (Pagewidth)))


/**
 * \param Addr Page start.
 * \param Pagewidth The width of the page to be deallocated
 * As if calling munmap(addr, size)
 * \brief Deallocates a single page, ignore owner.
 **/
#define UNMAPNOPID(Addr, Pagewidth) \
(mgsim_control((Addr), MGSCTL_TYPE_MEM, MGSCTL_MEM_UNMAP, (Pagewidth)))


/**
 * \param Pid The pid to prepare.
 * Sets the PID for an allocation/deallocation call.
 **/
#define DOPID(Pid) \
mgsim_control((Pid),  MGSCTL_TYPE_MEM, MGSCTL_MEM_EXTRA, MGSCTL_MEM_EXTRA_SETPID);


/**
 * \param Addr The starting address.
 * \param Pagewidth The width of the allocated page.
 * Allocates memory owned by the PID set via DOPID.
 *
 *
 * mmap(addr, size, pid), haal pid uit CPU
 **/
#define MAPONPID(Addr, Pagewidth) \
(mgsim_control((Addr), MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP_WITH_PID, (Pagewidth)))

/**
 * \param Pid The owner of the to be deallocated memory.
 * Deallocate all memory owned by Pid.
 * munmap(pid)
 **/
#define UNMAPONPID(Pid) \
(mgsim_control((Pid),  MGSCTL_TYPE_MEM, MGSCTL_MEM_EXTRA, MGSCTL_MEM_EXTRA_UNMAP_BY_PID))


#endif
