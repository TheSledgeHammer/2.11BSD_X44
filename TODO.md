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
	
## arch:
- i386/x86: (Merged under i386)
	- smp/multi-cpu:
		- machine-independent code: 90% complete
			- smp-related methods for cpu
		- machine-dependent code: 75% complete
			- boot sequence: cpu with lapic, ioapic & percpu
			- ipi
			- apic vectors/ IDTVEC's
				- apicvec.s: apicintr's 
				- cpu_info: CPUVAR(idepth, ilevel & ipending)
			- tsc

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md
	
## dev:
- Essential Driver Support:
	- usb: 								Work in progress
		- add: vhci, xhci
	- wscons/pccons:						Work in progress
		- double check wscons for errors/mistakes
- cfops update:
	- double check com.c needs cfdriver declaration
	- to use detach & activate routines
	
## fs:


## lib:
- libsa:
- libkern:
- x86emu:
	
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
	- i386:
		- pmbr
		- pxeldr

## ufs:

## vm:

## Other:
- Memory Segmentation (Software):
	- Seperate Process Segments: text, data, stack
	- Seperate Instruction & Data Spaces
