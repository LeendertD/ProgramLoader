#ifndef H_XDNO
#define H_XDNO

#include <svp/mgsim.h>

#ifndef MGSCTL_MEM_MAP_WITH_PID
#define MGSCTL_MEM_MAP_WITH_PID 2
#endif
#ifndef MGSCTL_MEM_EXTRA 
#define MGSCTL_MEM_EXTRA 3
#endif
#ifndef MGSCTL_MEM_EXTRA_SETPID
#define MGSCTL_MEM_EXTRA_SETPID 0
#endif
#ifndef MGSCTL_MEM_EXTRA_UNMAP_BY_PID
#define MGSCTL_MEM_EXTRA_UNMAP_BY_PID 1
#endif

#define STEP() mgsim_control(0, MGSCTL_TYPE_STATACTION, MGSCTL_SA_EXCEPTION, 0)

// mmap(addr, size, pid = 0)
#define MAPNOPID(addr, pagewidth) \
(mgsim_control((addr), MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP, (pagewidth)))


// munmap(addr, size)
#define UNMAPNOPID(addr, pagewidth) \
(mgsim_control((addr), MGSCTL_TYPE_MEM, MGSCTL_MEM_UNMAP, (pagewidth)))


// set pid in CPU
#define DOPID(pid) \
mgsim_control((pid),  MGSCTL_TYPE_MEM, MGSCTL_MEM_EXTRA, MGSCTL_MEM_EXTRA_SETPID);


// mmap(addr, size, pid), haal pid uit CPU
#define MAPONPID(addr, pagewidth) \
(mgsim_control((addr), MGSCTL_TYPE_MEM, MGSCTL_MEM_MAP_WITH_PID, (pagewidth)))

// munmap(pid)
#define UNMAPONPID(pid) \
(mgsim_control((pid),  MGSCTL_TYPE_MEM, MGSCTL_MEM_EXTRA, MGSCTL_MEM_EXTRA_UNMAP_BY_PID))


#endif
