TODO:

Kern:
- sys_kern.c (used? or unused?)
- sys_net (ubadr_t)
- uipc_syscalls.c

- user.h: 
	- u_ar0: redefine to machine-dependent reg.h
	- short	u_uisa[16];		/* segmentation address prototypes */
	- short	u_uisd[16];		/* segmentation descriptor prototypes */
	- char	u_sep;			/* flag for I and D separation */
	- struct u_ovd			/* automatic overlay data */
	- u_fps: floating point 
	- u_pcb: machine-dependent pcb.h
	- struct fperr u_fperr;				/* floating point error save */
	- remove duplicate and/or un-needed references (kinfo_proc)
	
Ufs:
- Add UFS
- WABL (NetBSD)

i386: (4.4BSD-Lite2)
- Doesn't contain a bootloader/ bootrom
- Only i386 relavent code to load the kernel 

Bootloader: (FreeBSD 3.0)
- Easy to Understand. Includes BTX Loader
- Provides basic bootloader for i386 & others in Future
- Easier to merge with existing 2.11/4.4BSD code
- Maintain a.out, coff & elf support

Possible Generic Header: 
(2.11BSD):
/* 2.11BSD: (file),v (version) (company) (date) */

(Other BSD's):
/* (Distro): (file),v (version) (company) (date) */

/* Applicable Copyright Notice Here */
