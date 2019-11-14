TODO:
- Check 2.11BSD original: rusage & k_rusage refs
- Adjust 2.11BSD new: if they match-up where appropriate
- Check for inodes in kernel to replace

Kern:
- int_sysent.c (systemcalls: vnodes, vfs, sockets update)
- kern_clock.c (check a few functions/pointers exist)
- kern_descrip.c (check a few functions/pointers exist)
- kern_exec.c (kern_exec2.c replaces this)
- kern_exit.c (proc[])
- kern_fork.c (swapout(rpp, X_DONTFREE, X_OLDSIZE, X_OLDSIZE))
- kern_sysctl.c (inode_sysctl, vm_sysctl)
- subr_xxx.c (networking)
- sys_kern.c (inodes)
- sys_pipes.c (other uses?)
- sys_process.c (ptrace)

Haven't really been looked at
- tty (all source files)
- uipc (all source files)

VM:

FreeBSD 3.0:
- BTX Bootloader

From Plan 9:
- edf.c, edf.h

Refer to:
Plan 9:
 plan9/sys/src/9/pc/mem.h
 plan9/sys/src/9/pc/mmu.c 		(initiates memory pool)
 plan9/sys/src/9/port/ucallocb.c
 plan9/sys/src/9/port/allocb.c 
