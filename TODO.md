A list of what needs work in no order. To obtain a successful build.
NOTE: Don't hesitate to add to this list. :)

#TODO:
##General:
- Cross-Compiler
	- Build Toolchain
- Makefiles:
	- Use .mk for definitions
	- fix multiple generic definitions: e.g. MACHINE ARCHDIR etc
		- i.e. lib
- Bug Fixes/ missing critical content

# usr/ (User & OS Libraries):
## lib:
- libkvm
		
## libexec:
		
# usr/sys/ (Kernel):
## conf:
- check: setup is correct

## kern:
- gprof.h & gmon.h: (both look to do the same thing)
	- fix double up
	
## arch:
- i386/x86: (Merged under i386)
	- conf
	- cpu_info
		- cpu_setup: basics implemented
	- pae: implemented but not fully integrated
		- missing in machdep.c & locore.s
	- devices: 
		- see below: dev

## devel: (planned)
- Code planned for future integration
- See devel folder: README.md & TODO.md
- Touch up (job_pool, wqueue & tasks) intergration with threadpools

## dev:
- devices:

## lib:
	
## net / netimp / netinet / netns:
Of Interest Todo:
- 2.11BSD's networking stack
	- Needs Updating to support: i.e. ipv6, firewall/packet filter

## stand:
- boot:
	- efi support: needs work
	- commands: needs work
	- install: not present
	- efi:
		- libefi & loader: Update From FreeBSD to be NetBSD/2.11BSD compatable
	### arch:
	- i386: 			
		- gptboot
		- isoboot
		- loader:
			- loader.conf: incomplete
		- pmbr
		- pxeldr

## ufs:

## vfs:

## vm:

