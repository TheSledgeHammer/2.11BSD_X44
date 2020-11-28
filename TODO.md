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
- exec_macho: exec_mach_copyargs 
	(NetBSD 5.2: /compat/mach/mach_exec.c)
	
## arch:
- i386/x86: (Merged under i386)
	- conf
	- devices: 
		- see below: dev

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md

## dev:
- No devices, only basic (needs a working compiler)

## lib:
	
## net / netimp / netinet / netns:
Of Interest Todo:
- 2.11BSD's networking stack
	- Needs Updating to support: i.e. ipv6, firewall/packet filter

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

## vfs:

## vm:


Memory Segmentation (Hardware): CPU Registers
Memory Segmentation (Software):
Seperate Process Segments: text, data, stack
Seperate Instruction & Data Spaces
