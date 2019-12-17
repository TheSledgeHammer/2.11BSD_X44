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
- tty_conf.c
- tty_pty.c
- uipc_syscalls.c

Vm:
- VM using FreeBSD 2.0 Code Base: May need to move VM code base to 4.4BSD
- If wish to implement Memory Management Idea (w/o hassles)

- vmmeter.h (belongs in sys)
- vm.h (extended from 2.11BSD's)
2.11BSD's VM
- vmmac.h
- vmsystem.h
- vmparam.h
- vm_proc.c
- vm_sched.c
- vm_swap.c
- vm_swp.c

FreeBSD 3.0 or Higher:
- BTX Bootloader


Executables:  
__LDPGSZ = machine/exec.h