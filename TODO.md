TODO:
boot: (Focus on 2.11BSDx86 boot.)
- dev_net.c
- libi386
	- pxe
- NetBSD ia64: Uses parts of FreeBSD's boot
	- Should update parts from FreeBSD

i386:
- locore.s: icode 

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
	- kinfo_proc: could be useful unless it's superceded by (ktrace or dtrace?) 
	
UFS, FFS & LFS:
- extattr, dirhash
- dinode: ufs1 & ufs2
- WABL (NetBSD)

Possible Generic Header: 
(2.11BSD):
/* 2.11BSD: (file),v (version) (company) (date) */

(Other BSD's):
/* (Distro): (file),v (version) (company) (date) */

/* Applicable Copyright Notice Here */
