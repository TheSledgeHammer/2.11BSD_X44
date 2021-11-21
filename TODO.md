A General todo list. Don't hesitate to add to this list. :)

# TODO:
## Compiler:
### Tools:
- Needs Fixing:
	- Disklabel Associated
		- mklocale
		- disklabel
		- fdisk
		- boot0cfg
	- btxld
	- zic
- Unimplemented:
	- rpcgen
			
### Kernel:
- Building Kernel/Arch:
	- Config:
	- Compile:
		- numerous compilation errors 

# usr/ (User & OS Libraries):
## lib:

## libexec:

## sbin:

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
