TODO:
-lf_advlock: Used in Filesystems

i386:
- machdep.c
- pmap.c
- locore.s
- trap.c: p->p_usrpri: doesn't exist in 2.11BSD
- vm_machdep.c: u->u_procp->p_p0br??

Kern:
- longjmp, setjmp
- libkern.h (NetBSD/ OpenBSD): KASSERT, assert etc..
- rwlock.h
- tasks.h (OpenBSD)
- init_main.c
	- lines 250 to 262
	- uipc_mbuf.c: mbinit
	- consinit
	- cpu_startup
	- start_init(curproc, framep);
- if INET: remanents of 2.11BSD's networking stack overlay
- kern_clock.c
- sys_kern.c (used? or unused?)
- uipc_syscalls.c

- user.h: 
	- u_ar0: redefine to machine-dependent reg.h
	- short	u_uisa[16];					/* segmentation address prototypes */
	- short	u_uisd[16];					/* segmentation descriptor prototypes */
	- char	u_sep;						/* flag for I and D separation */
	- struct u_ovd						/* automatic overlay data */
	- u_fps: floating point 
	- u_pcb: machine-dependent pcb.h
	- struct fperr u_fperr;				/* floating point error save */
	- remove duplicate and/or un-needed references (kinfo_proc)
	
UFS, FFS & LFS:
- extattr, dirhash
- dinode: ufs1 & ufs2
- WABL (NetBSD)

i386: (4.4BSD-Lite2)
- Doesn't contain a bootloader/ bootrom
- Only i386 relavent code to load the kernel 

Bootloader: (FreeBSD 5.0)
- NetBSD ia64: Uses most parts of boot/common (Compatability)
- Easier to Understand. Includes BTX Loader
- Provides basic bootloader for i386 & others in Future
- Easier to integrate with 2.11/4.4BSD code

- Eventually convert to sysBSD project

Possible Generic Header: 
(2.11BSD):
/* 2.11BSD: (file),v (version) (company) (date) */

(Other BSD's):
/* (Distro): (file),v (version) (company) (date) */

/* Applicable Copyright Notice Here */
