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
	- npx.c: update
	- Move to /dev: (update dependencies so this is possible)
		- spkr.c, spkr.h, spkrreg.h, timerreg.h

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md

## dev:
- Clean up: A little messy
	- com
- Essential Driver Support:
	- usb
	- disk	(ata & scsi)
	- video
	- mouse/keyboard
- move all machine-dependent code to that arch (that isn't portable)

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
	- ufs2
	- journaling
	
## vfs:

## vm:

## Other:
- Memory Segmentation (Hardware): CPU Registers
- Memory Segmentation (Software):
- Seperate Process Segments: text, data, stack
- Seperate Instruction & Data Spaces
