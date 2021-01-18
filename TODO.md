A General todo list. Don't hesitate to add to this list. :)

# TODO:
## General:
- Compiler
- Makefiles
- Bug Fixes &/or missing critical content


# usr/ (User & OS Libraries):
## lib:
- libkvm
		
## libexec:
		
# usr/sys/ (Kernel):
## conf:

## kern:
- ksyms: NKSYMS (default is 0 ???)
- exec_macho: exec_mach_copyargs 
	(NetBSD 5.2: /compat/mach/mach_exec.c)
	
## arch:
- i386/x86: (Merged under i386)
	- Move to /dev: (update dependencies so this is possible)
		- spkr.c, spkr.h, spkrreg.h, timerreg.h

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md

- dev/kbd: current implementation: ((WILL NOT WORK AS IS!!!) Would require the entire device layout to be re-implemented around FreeBSD)
	- no virtual keyboard in kernel (deprecated)
	- resource maps: change from FreeBSD/ DragonflyBSD's to NetBSD style
	- keyboard drivers & config driver (cfdriver) are not correct. example: genkbd.c
		- current cfdriver/data structure: 
			- cannot manage multiple config drivers through a single driver interface as implemented
			by FreeBSD/ DragonflyBSD keyboard.
		- each cfdriver/data structure:
			- Needs a device reference and the following information
				- devs (if applicable), name, match, attach, flags, and size
	- kbd changes:
		- remove need for the in-kernel virtual keyboard driver (used with FreeBSD's sysinit)
		- provide a generic keyboard layer with routines that can manage multiple.
		OR
		- At minimum a machine-independent keyboard driver with PS/2 compatability
		- cfdriver is only used with the driver not the generic keyboard layer
	
## dev:
- Essential Driver Support:
	- usb: 								Work in progress
	- disk	(ata, atapi, ahci & scsi): 	Work in progress
	- video:								Work in progress
	- mouse/keyboard:						Work in progress

## lib:
	
## net / netimp / netinet / netns:
Of Interest Todo:
- 2.11BSD's networking stack
	- No Support for ipv6, firewall/packet filter plus much more

## stand:
- boot:
	- efi
	- commands: needs work
	- install: not present
	### arch:
	- i386:
		- gptboot
		- isoboot
		- pmbr
		- pxeldr

## ufs:
- ufs/ffs:
	- dirhash
	- ufs2/ffs2 support: Work in Progress
		- Fix ffs/fs.h: To account for ufs1 & ufs2
			- #define blksize(fs, ip, lbn) 
	- journaling
	
## vfs:

## vm:

## Other:
- Memory Segmentation (Hardware): CPU Registers
- Memory Segmentation (Software):
- Seperate Process Segments: text, data, stack
- Seperate Instruction & Data Spaces
