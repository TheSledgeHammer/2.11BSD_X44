TODO:
- rename: boot/libsa/stand.h, reduce conflict with lib/libsa/stand.h

- pmap:  rewrite to work with cpt
	- pmap_pte
	- pmap_enter
	- pmap_remove


boot: (Focus on 2.11BSDx86 boot.)
- dev_net.c
- libi386
	- bootinfo32
	- pxe
- NetBSD ia64: Uses parts of FreeBSD's boot
	- Should update parts from FreeBSD

i386:
- machdep.c (incomplete): 
- locore.s 
- cpufunc.h
- asm.h
- db_machdep
- math
- sys_machdep update: fsbase, ioperm, ldt, pcb_extend
- proc_machdep (netbsd)
- Follow FreeBSD 5 for optimal compat with current architechture state & the bootloader

libsa:
- bootparam

Kern:
- p->p_usrpri: doesn't exist in 2.11BSD (needs a solution)
- vm_machdep.c: u->u_procp->p_p0br??
- init_main.c
	- lines 250 to 262 (references to old vm startup)
	- uipc_mbuf.c: mbinit
	- consinit
	- cpu_startup
- if INET: remanents of 2.11BSD's networking stack overlay
- kern_clock.c: no statclock
- sys_kern.c (used? or unused?)
- uipc_syscalls.c: missing syscallargs

- user.h: 
	- u_ar0: redefine to machine-dependent reg.h
	- short	u_uisa[16];					/* segmentation address prototypes */
	- short	u_uisd[16];					/* segmentation descriptor prototypes */
	- char	u_sep;						/* flag for I and D separation */
	- struct u_ovd						/* automatic overlay data */
	- u_fps: floating point 
	- u_pcb: machine-dependent pcb.h
	- remove duplicate and/or un-needed references (kinfo_proc)
	- kinfo_proc: could be useful unless it's superaceded by (ktrace or dtrace?) 
	
UFS, FFS & LFS:
- extattr, dirhash
- dinode: ufs1 & ufs2
- WABL (NetBSD)

i386: (4.4BSD-Lite2)
- Doesn't contain a complete bootloader
- Only i386 relavent code to load the kernel
- Not sure what is missing (excluding being able to bootstrap)  

Possible Generic Header: 
(2.11BSD):
/* 2.11BSD: (file),v (version) (company) (date) */

(Other BSD's):
/* (Distro): (file),v (version) (company) (date) */

/* Applicable Copyright Notice Here */
