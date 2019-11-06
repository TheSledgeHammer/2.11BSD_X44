TODO:
- Check 2.11BSD original: rusage & k_rusage refs
- Adjust 2.11BSD new: if they match-up where appropriate
- Check for inodes in kernel to replace


Kern:
- int_sysent.c (systemcalls: vnodes, vfs)
- kern_clock.c
- kern_descrip.c
- kern_exec.c
- kern_exit.c
- kern_fork.c
- kern_sysctl.c
- subr_xxx.c
- sys_kern.c
- sys_pipes.c (deprecated, vnodes use sockets)
- sys_process.c

Haven't really been looked at
- tty (all source files)
- uipc (all source files)

VM:


From Plan 9:
- Pool.c, Pool.h, malloc.c, ucalloc.c, ucallocb.c, allocb.c, alloc.c, xalloc.c: Uses a tree structure. 
- edf.c, edf.h

Refer to:
Plan 9:
 plan9/sys/src/9/pc/mem.h
 plan9/sys/src/9/pc/mmu.c 		(initiates memory pool)
 plan9/sys/src/9/port/alloc.c
 plan9/sys/src/libc/port/malloc.c 
 plan9/sys/src/9/port/ucalloc.c
 plan9/sys/src/9/port/xalloc.c 
