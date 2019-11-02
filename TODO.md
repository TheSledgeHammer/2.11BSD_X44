In Kernel:
TODO:
- Check 2.11BSD original: rusage & k_rusage refs
- Adjust 2.11BSD new: if they match-up where appropriate
Fix:
- kern_descrip.c
- kern_exec.c
- kern_sig.c

Contains Inodes:
- kern_descrip.c
- kern_exec.c
- kern_sig.c
- kern_sysctl.c
- sys_kern.c
- sys_pipe.c: May be deprecated with vnodes

Includes Inodes, but doesn't use
- subr_log.c: Vnodes is a drop-in?
- kern_fork.c
- kern_exit.c
- tty.c
- tty_pty.c
- tty_tb.c






From Plan 9:
- Pool.c, Pool.h, malloc.c, ucalloc.c, alloc.c, xalloc.c: Uses a tree structure. 
- edf.c, edf.h

Refer to:
Plan 9:
 plan9/sys/src/9/pc/mem.h
 plan9/sys/src/9/pc/mmu.c 		(initiates memory pool)
 plan9/sys/src/9/port/alloc.c
 plan9/sys/src/libc/port/malloc.c 
 plan9/sys/src/9/port/ucalloc.c
 plan9/sys/src/9/port/xalloc.c 
