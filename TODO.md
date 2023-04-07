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
## lib:

## libexec:

## sbin:

## share:

## tools:

## stand:
- boot:
	- setup configuration settings for elf & non-elf based kernels
	- add gfx_fb
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

## dev:
- usb (Disabled): Has several compiler errors.
- core:
	- isadma: does not work with current device arrangement
	
- Improve directory structure
- Essential Drivers
	- To Support:
		- AGP: Graphics
		- USB: Fix compiler errors and add xhci
		- ACPI

## fs:

## lib:
- libsa:
	- Add bzipfs, gzipfs, pkgfs, splitfs
- libkern:

## miscfs:

## net / net80211 / netinet / netinet6 / netipsec / netkey / netns / netpfil :
- Update to make use of tcp_checksum & udp_checksum
- netpfil: 
	- Add NPF
		
## ufs:
- implement Extended Attributes
- lfs:
	- update lfs structures for 64-bit.
		- dip switching depending on if UFS1/UFS2 is complete
	- improve logging facilities (see: NetBSD)
	- snapshots? 
- ufs:
	- journaling: softupdates + revised htbc
	- dirhash
- ufs211:
   	- fix quotas
- ufml:
	- adds User libs
	- improve fs support
	- add fs-based routines

## vm:

## Other:
- Memory Segmentation (Software):
	- Seperate Process Segments: text, data, stack
	- Seperate Instruction & Data Spaces
