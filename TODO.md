TODO:
- rename: boot/libsa/stand.h, reduce conflict with lib/libsa/stand.h
boot: 
- Focus on 2.11BSDx86 boot.
- FreeBSD's loading kern modules not fully implemented
- interp.c: interp_builtin_cmd (command set search)
- md.h: needs an updated stand.h 
- libi386
	- bootinfo32
	- pxe
- NetBSD ia64: Uses parts of FreeBSD's boot
	- Should update parts from FreeBSD

i386:
- machdep.c (incomplete)
- pmap.c (incomplete)
- locore.s (non-existent: depends on boot)
- GDT, LDT: non-existent
- p->p_usrpri: doesn't exist in 2.11BSD (needs a solution)
- vm_machdep.c: u->u_procp->p_p0br??

libsa:
- bootparam

Kern:
- longjmp, setjmp
- libkern.h (NetBSD/ OpenBSD): KASSERT, assert etc..
- init_main.c
	- lines 250 to 262 (references to old vm startup)
	- uipc_mbuf.c: mbinit
	- consinit
	- cpu_startup
	- start_init(curproc, framep);
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
