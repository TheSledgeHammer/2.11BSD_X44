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
	
## arch:
- i386/x86: (Merged under i386)

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md
	
## dev:
- Essential Driver Support:
	- usb: 							Work in progress
		- fix power: ehci, uhci & ohci
		- fix: ucom tty functions
		- add: vhci, xhci
	- wscons/pccons:					Work in progress
		- double check wscons for errors/mistakes

## fs:


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
- boot/arch:
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
