- boot: Check & create Makefiles if needed
	- common:
		- isapnp.c/.h
		- pnp.c 
	- efi:
		- libefi & loader: Update From FreeBSD to be NetBSD/2.11BSD compatable
	- i386: 			
		- gptboot
		- isoboot
		- loader:
		- pmbr
		- pxeldr

Boot Loading:
NetBSD: (i386/stand)
- exec.c
	- exec_multiboot
- exec_multiboot1.c
	- ksyms_addr_set (elf32 & elf64)
i386:
- multiboot.c
	- copy_syms
	- multiboot1_ksyms_addsyms_elf
- machdep.c
	- init386_ksyms
		
FreeBSD: (stand/i386)
- bootinfo64.c
- multiboot.c

Linker/Ksyms:
- Determines all the variables (what, where and how) it is executed.
- Includes and not limited to: modules & symbols(aka elf, ecoff, aout)

Linker(File Metadata) (FreeBSD): Simplified Overview: In actuality is more advanced and does more than described here.
(limited to only aout and elf formats)
Requirements:
	- ddb
	- kern_environment.c
	- kern_linker.c
initiates in kernel init_main after loading symbol table in machine

Ksyms (NetBSD): A kernel symbol and module loader for multiple exec formats(aka elf, ecoff, aout) 
Requirements:
	- ddb
	- kern_ksyms
initiates in machine machdep
