A General todo list. Don't hesitate to add to this list. :)

# TODO:
## Compiler:
### Tools:
			
### Kernel:
- Building Kernel/Arch:
	- Compile Issues/Errors:
		- Linker: None atm, Yay! :)
		- Compiler: None atm, Yay! :)
- Fix:
- Stand includes: Not being placed in
  correct destination directory
  during build process (should be
  in usr/includes not the root of
  the destination directory
- libpthread needs updating

### Documentation:
- Manpages:
	- Severely lacking current information
	OR
	- Missing entirely

# usr/ (User & OS Libraries):
## Contrib:
- GNU:
	- GCC:
		- libstdc++-v3 & libsupc++:
			- Compiler error with vterminate.cc. (More info to be provided ASAP)
				- The issue is related to line 32: # include <cstdio> 
	
## lib:
- csu:
	- crtbegin.c: jcr and jv_RegisterClasses does not work correctly.
 	- elf_note: unused
- libc:
	- net: nslexer.l doesn't like %%
	- softfloat: add to libc
	- stdio: improve support
 		- wide characters.

## libexec:
- add:
	- Network:
		- rbootd, rexecd, rlogind, rshd, telnetd

## sbin:
- add: More critical software
	- Generic:
	- Filesystem:
	- Network:
		- startslip

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
- Add: More critical software
	- Generic:
		- calendar, tr
	- Filesystem:
		- quota
	- Network:
		- rlogin, rs, rsh, ruptime, rwho, telnet

## usr.lib:
- add:
	- Network:
		- libtelnet

## usr.sbin:
- pstat: add proc and usr stats
- mkpasswd: ndbm (4.4BSD/NetBSD) is not fully compatible.
	- Still relies on dbm_pagfno from 2.11BSD's ndbm
- add: More critical software
	- Generic:
		- amd, eeprom, kgmon, syslog
	- Filesystem:
		- edquota, quot, quotaon, repquota
	- Network:
		- altq, inetd, rwhod, trpt, trsp

# usr/sys/ (Kernel):
## conf:
- Seperate file for conf options

## ddb:

## kern:
- Implement Access Control Lists
- Implement Extended Attributes

- subr_hints.c: It's usefulness is in question...
- replace use of spl with something more like DragonflyBSD's lwkt tokens.
- RLimit (Minor Issue)
	- 4.4BSD-Lite2 & 2.11BSD conflict.
	- init_main.c: plimit pointer to rlimit and user pointer to rlimit
	
## arch:
- create seperate options file for each arch
- i386/x86: (Merged under i386)
	- pnpbios: add devices
	- softintr: add feature to smart select soft-interrupts over hardware based interrupts instead of using "__HAVE_GENERIC_SOFT_INTERRUPTS"

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md
	- AdvVM: 
		- add a device mapper, use NetBSD's dm as reference.
	- HTBC: 
		- Redo: implements only the blockchain and caching.
		- Logging/Journal functions are filesystem dependent
	- Overlays: pmap's and map improvements
	- Threads: add workqueues and threadpools into kernel

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
- NFS: Update Me, I'm not working yet!!

## net / net80211 / netinet / netinet6 / netipsec / netkey / netns :
		
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
