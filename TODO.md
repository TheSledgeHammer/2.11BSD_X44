A General todo list. Don't hesitate to add to this list. :)

# TODO:
## Compiler:
### Tools:
- Needs Fixing:
	- Disklabel Associated
		- disklabel
		- fdisk
- Unimplemented:
	- rpcgen
			
### Kernel:
- Building Kernel/Arch:
	- Compile Issues/Errors:
		- Linker: None atm, Yay! :)
		- Compiler: None atm, Yay! :)
- Config: Fix and update arch/conf.c.
	- Device driver's setup, that need-count and may not have any initalized device. 
	- current examples: video, pc speaker, apm & tabldisc

# usr/ (User & OS Libraries):
## Contrib:
	- Add Lua
	
## lib:

## libexec:

## sbin:

## share:

## tools:

## stand:
- boot:
	- setup configuration settings for elf & non-elf based kernels
	- add: lua interpreter support
	- efi:
		- update missing.
		- needs acpica.

## usr.bin:

## usr.lib:
- libkvm: missing functions

## usr.sbin:

# usr/sys/ (Kernel):
## conf:
- Seperate file for conf options

## ddb:

## kern:
- subr_hints.c: It's usefulness is in question...
- replace use of spl with something more like DragonflyBSD's lwkt tokens.
- RLimit (Minor Issue)
	- 4.4BSD-Lite2 & 2.11BSD conflict.
	- init_main.c: plimit pointer to rlimit and user pointer to rlimit
	
## arch:
- create seperate options file for each arch
- i386/x86: (Merged under i386)
	- pnpbios: add devices

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md
	- AdvVM: add a device mapper, use NetBSD's dm as reference.
	- Overlays: pmap's and map improvements
	- Threads: add workqueues and threadpools into kernel
	- MPX: mpx subroutines need improving, if mpx styled threads are to work!?

## dev:
- usb (Disabled): Has several compiler errors.
- core:
	
- Improve directory structure
- Essential Drivers
	- To Support:
		- USB: Fix compiler errors and add xhci
		- ACPI: To implement

## fs:
- add UDF support

## lib:
- libsa:
	- Add bzipfs, gzipfs, pkgfs, splitfs
- libkern:
- tekken:
	- Support FreeBSD's tekken library?
	- Better Terminal Emulation support.
		- Could work with wscons

## miscfs:
- add NFS: Can actually support it now. With network functionality implemented!!

## net / net80211 / netinet / netinet6 / netipsec / netkey / netns / netpfil :
- Update to make use of tcp_checksum & udp_checksum
- netpfil: 
	- NPF: Ported from NetBSD 6.x
		- Needs alot of changes to actually make it work. 
		
## ufs:
- implement Extended Attributes
- lfs:
	- update lfs structures for 64-bit. Some still only 32-Bit
	- improve logging facilities (see: NetBSD)
	- snapshots? 
- ufs:
	- journaling: softupdates + revised htbc
	- dirhash
- ufs211:
   	- fix quotas
- ufml:
	- adds User library.
	- improve fs support
	- add fs-based routines

## vm:
- Pseudo-Segment: take over of the vmspace stack management.
	- Fixing grow to work with pseudo-segmentation or providing a temporary solution.

## Other:
- Memory Segmentation (Software):
	- Seperate Process Segments: text, data, stack
	- Seperate Instruction & Data Spaces
