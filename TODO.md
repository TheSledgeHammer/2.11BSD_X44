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


Executables:
Combining: 
- NetBSD's API (exec: aout, ecoff & elf)
- FreeBSD's API (imgact: aout & elf)

- FreeBSD provides better integration in kernel & vmspace (my opinion)
- NetBSD provides better api support such as coff, ecoff, mach-o & xcoff (Depends on IBM's licencing of it's API??) 
- Exchange the use of NetBSD's VMCMD with FreeBSD's vmspace and limits
- Removes need for exec_script.h
- Only Exception: NetBSD's exec_elf32.c & exec_elf64.c.  

__LDPGSZ = machine/exec.h