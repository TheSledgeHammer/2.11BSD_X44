TODO:
- Makefiles
- boot:
	- common:
		- isapnp.c/.h
		- pnp.c 
	- efi
	- usb
	- dloader
	- i386:
		- libi386:
		- cdboot
		- gptboot
		- isoboot
		- loader:
			- Makefile
			- Makefile.depend
		- pmbr
		- pxeldr

- arch:
	- i386:
		- locore
		- bios
		- vm86

- ufs:
	- ext2fs (NetBSD)
	- ufs:
		- ufs1 & ufs2?
		- dirhash
		- extattr
		- wapbl
			
- vfs:
	- deadfs
	- ufs211:
		- extern
		- vnops
		- vfsops


md_init & sysinit are not possible atm, they are intertwined with FreeBSD's linker_set & linker (executables), with many in kernel modules.
Hence a massive amount of code injection/ rewrite would be needed in order to set this up correctly.
		
Kern:
- p->p_usrpri: doesn't exist in 2.11BSD (needs a solution)
- vm_machdep.c: u->u_procp->p_p0br??
- init_main.c
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


Possible Generic Header: 
(2.11BSD):
/* 2.11BSD: (file),v (version) (company) (date) */

(Other BSD's):
/* (Distro): (file),v (version) (company) (date) */

/* Applicable Copyright Notice Here */
