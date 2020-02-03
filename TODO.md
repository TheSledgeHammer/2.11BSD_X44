TODO:

kern_exec:
- Doesn't need NetBSD kern_exec lines 430 - 463. Filled out by below
- exec_new_vmspace:
	- could be filled in further from NetBSD's kern_exec lines 394 - 430
- exec_extract_strings:
	- could use kern_exec line 440
-Complete kern_exec

Kern:
- sys_kern.c (used? or unused?)
- sys_net (ubadr_t)
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
- Easier to Understand. Includes BTX Loader
- Provides basic bootloader for i386 & others in Future
- Easier to integrate with 2.11/4.4BSD code

- Eventually convert to sysBSD project
- Format Support: Use exec format + bsd: 
- e.g elf_netbsd or elf_freebsd. To support imgact & vmcmd
- no reason not to keep support for aout & others
- fs: ufs, ext, zfs, hammer
support for zfs & hammer likely to be last. unless someone else provides.

Possible Generic Header: 
(2.11BSD):
/* 2.11BSD: (file),v (version) (company) (date) */

(Other BSD's):
/* (Distro): (file),v (version) (company) (date) */

/* Applicable Copyright Notice Here */
