A General todo list. Don't hesitate to add to this list. :)

# TODO:
## General:
- Compiler
- Build Toolchain
- Makefiles:
	- Use .mk for definitions
- Bug Fixes/ missing critical content

# usr/ (User & OS Libraries):
## lib:
- libkvm
		
## libexec:
		
# usr/sys/ (Kernel):
## conf:

## kern:
	
## arch:
- i386/x86: (Merged under i386)
	- conf
	- pae: implemented but not fully integrated
		- missing in machdep.c & locore.s
	- devices: 
		- see below: dev

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md & TODO.md

## dev:
- No devices, only basic (to get working compiler)

## lib:
	
## net / netimp / netinet / netns:
Of Interest Todo:
- 2.11BSD's networking stack
	- Needs Updating to support: i.e. ipv6, firewall/packet filter

## stand:
- boot:
	- efi: needs work
		- Not compatable with 2.11BSD 
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
