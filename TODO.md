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

## sbin:
- fsck: replace references to ufs_daddr_t
		
# usr/sys/ (Kernel):
## conf:

## kern:
- sys_generic.c: fileops: fo_rw
- fileops: update fileop structures to make use of kqfilter
- kern_resource.c:
	- missing methods
	
## arch:
- i386/x86: (Merged under i386)
	- clockframe usage (machine/cpu.h)

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md
	
## dev:
- Essential Driver Support:
	- usb: 							Work in progress
		- fix power: ehci, uhci & ohci
		- add: vhci, xhci
	- wscons/pccons:					Work in progress
		- double check wscons for errors/mistakes

## fs:


## lib:
- libsa:
	- stand.h
		- add relavent libsa methods
	
## net / netimp / netinet / netns:
Of Interest Todo:
- 2.11BSD's networking stack
	- No Support for ipv6, firewall/packet filter plus much more

## stand:
- boot:
	- configurable
		- boot format: FreeBSD Slices & NetBSD Adaptor & Controller
			- failsafe: Cannot boot if both are true or both are false
	- efi
	- commands: needs work
	- install: not present
- boot/arch:
	- loader/main.c: PSL_RESERVED_DEFAULT = PSL_MBO??
	- i386:
		- gptboot
		- isoboot
		- pmbr
		- pxeldr

## ufs:

## vm:

## Other:
- Memory Segmentation (Software):
	- Seperate Process Segments: text, data, stack
	- Seperate Instruction & Data Spaces
