A General todo list. Don't hesitate to add to this list. :)

# TODO:
## General:
- Compiler
- Makefiles
- Bug Fixes &/or missing critical content

## Share/mk & mkhost:
- Seperate Host & Target (2.11BSD_X44) mk files
- build.sh:
	- Allow the following parameters to set
	- Compiler (GCC or Clang)
	- Object Format (ELF, a.out)
		- Restrict object formats that won't run on a given arch(i.e. x86_64 with a.out)
- Todo:
	- bsd.sys, sys
	- bsd.prog.mk:
		- __bsd_prog_mk_system_libs: remove unnecessary libs
	
- Host Environment:
	- 1:1 format.
	- Target provides necessary defitions
- Forseeable Issues: (with above layout)
	- Compilation order & variables
		- #1 Simple Solution: 
			- A definitions file Of static/dynamic variables
				- For Example:
					- HOSTSRCDIR = TARGETSRCDIR 	/* targets source files needed by host */
					- HOSTCC = TARGETCC 				/* target c compiler */
					- HOSTCPP = TARGETCPP 			/* target cpp compiler */ 

## Tools (NetBSD):
- Only import required for current successful compilation
	- Can add more as needed
- Changes to NetBSD's Toolchain:
	- Support: Clang and GCC
	- Have all compiler components under a single directory.
		-  E.g. Compiler Folder:
			- GCC: binutils, flex(optional), gcc, gdb, gmake, groff, m4, mpc, mpfr
			- Clang: clang, llvm, compiler-rt
- add Makefiles
- Long-Term Goal: 
	- Convert this to a gradle, cmake or meson build toolchain/cross-compiler

# usr/ (User & OS Libraries):
## lib:
- libkvm
		
## libexec:

## sbin:
- fsck: replace references to ufs_daddr_t
		
# usr/sys/ (Kernel):
## conf:

## kern:
- event/kqfilter: implemented but unused.
	
## arch:
- i386/x86: (Merged under i386)
	- smp/multi-cpu:
		- machine-independent code: 90% complete
			- smp-related methods for cpu
		- machine-dependent code: 75% complete
			- boot: considering FreeBSD's mpboot.s
			- smp: alloction to assign interrupts to CPUs
			- tsc: missing struct timecounter

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md
	
## dev:
- Essential Driver Support:
	- usb: 								Work in progress
		- add: vhci, xhci
	- wscons/pccons:						Work in progress
		- double check wscons for errors/mistakes
- cfops update:
	- double check com.c needs cfdriver declaration
	- to use detach & activate routines
	
## fs:


## lib:
- libsa:
- libkern:
- x86emu:
	
## net / netimp / netinet / netns:
Of Interest Todo:
- 2.11BSD's networking stack
	- To Support:
		- ipv6
		- firewall/packet filter
		- plus much more

## stand:
- boot:
	- configurable
		- boot format: FreeBSD Slices & NetBSD Adaptor & Controller
			- failsafe: Cannot boot if both are true or both are false
	- efi
	- commands: needs work
	- install: not present
- boot/arch:
	- i386:
		- pmbr
		- pxeldr

## ufs:

## vm:

## Other:
- Memory Segmentation (Software):
	- Seperate Process Segments: text, data, stack
	- Seperate Instruction & Data Spaces
