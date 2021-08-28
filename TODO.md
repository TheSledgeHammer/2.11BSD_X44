A General todo list. Don't hesitate to add to this list. :)

# TODO:
## General:
- Compiler
- Makefiles
- Bug Fixes &/or missing critical content

## Compiler/Tools:
- Can't Compile
	- pax
	- tic
- Tools to Remove (Alternative)
	- Groff (Mandoc)
- Unimplemented:
	- LLVM/Clang
	- rpcgen
	- GDB	(Would be nice to remove)
	- GCC	(Would be nice to remove)
	- pigz
	- makefs
	- date
- To Add (when needed)
	- xz
	- zic

# usr/ (User & OS Libraries):
## lib:

## libexec:

## sbin:
- fsck: replace references to ufs_daddr_t

## share:

## tools:

## usr.bin:

## usr.lib:
- libkvm

## usr.sbin:

# usr/sys/ (Kernel):
## conf:

## kern:
- event/kqfilter: implement in device io
- diskslices
	
## arch:
- i386/x86: (Merged under i386)
	- swapgeneric.c: Update... Contains deprecated code.

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md
	
## dev:
- Essential Driver Support:
	- usb:
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
