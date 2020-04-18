TODO:
- Build Toolchain (Cross-Compile): NetBSD's
	- Attempt build on NetBSD (Can't build on Linux without a Toolchain)
	- Makefiles
- Merge KASSERT CTASSERT into ASSERT

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
		- bios
		- vm86

- ufs:
	- ext2fs (NetBSD)
	- ufs:
		- ufs1 & ufs2?
		- dirhash
		- extattr
		- wapbl
			
Kern:
- kern_fork: newproc
- vm_machdep.c: u->u_procp->p_p0br: (no reference in 4.4BSD-Lite2)
	- 4.3BSD Reno/ 4.4BSD Remanent: once in struct proc. Obsolete?? 
- if INET: remanents of 2.11BSD's networking stack overlay (keep in place for now)
- syscallargs: To be fixed: temporary compat with 4.4BSD-Lite2

- user.h: 
	- short	u_uisa[16];					/* segmentation address prototypes */
	- short	u_uisd[16];					/* segmentation descriptor prototypes */
	- char	u_sep;						/* flag for I and D separation */
	- struct u_ovd						/* automatic overlay data */
	- remove duplicate and/or un-needed references (kinfo_proc)
	- kinfo_proc: could be useful unless it's superceded by (ktrace or dtrace?) 

Memory Segmentation (Hardware): CPU Registers
Memory Segmentation (Software):
Seperate Process Segments: text, data, stack
Seperate Instruction & Data Spaces

md_init & sysinit are not possible atm, they are intertwined with FreeBSD's linker_set & linker (executables), with many in kernel modules. Hence a massive amount of code injection/ rewrite would be needed in order to set this up correctly.
NetBSD: has an equivalent linker (executables) in userspace. May cause more problems than it solves if moved to kernelspace.

Possible Generic Header: 
(2.11BSD):
/* 2.11BSD: (file),v (version) (company) (date) */

(Other BSD's):
/* (Distro): (file),v (version) (company) (date) */

/* Applicable Copyright Notice Here */
