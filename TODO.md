TODO:

Kern:
- kern_clock.c (check a few functions/pointers exist)
- kern_descrip.c (check a few functions/pointers exist)
- kern_exit.c (proc[])
- kern_fork.c (swapout(rpp, X_DONTFREE, X_OLDSIZE, X_OLDSIZE))
- kern_sysctl.c (inode_sysctl, vm_sysctl)
- subr_xxx.c (networking)
- sys_kern.c (inodes)
- sys_process.c (ptrace)

Haven't really been looked at
- tty (all source files)
- uipc (all source files)

VM: (Lowest Priority ATM: Kernel & TTY First)
- Better Memory Management Needed

FreeBSD 3.0:
- BTX Bootloader