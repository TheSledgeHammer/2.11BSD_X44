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
- event/kqfilter: implemented but unused.
- diskslice:
	- devsw: lines 525, 567, 643, 725 & 728
	
## arch:
- i386/x86: (Merged under i386)
	- swapgeneric.c: Update... Contains deprecated code.
	- Update IO(pci, eisa, etc): To use the intr routines over the isa_intr routines

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md
	
## dev:
- Essential Driver Support:
	- usb: 								Work in progress
		- add: vhci, xhci
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
	- efi: For common efi code.
		- Makfiles:
			- Adjust makefiles. 
		- loader:
			- efiserialio.c
			- missing efi commands
	- commands: needs work
	- install: not present

## ufs:

## vm:

## Other:
- Memory Segmentation (Software):
	- Seperate Process Segments: text, data, stack
	- Seperate Instruction & Data Spaces
