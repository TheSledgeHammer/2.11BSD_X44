A General todo list. Don't hesitate to add to this list. :)

# TODO:
## Compiler:
### Tools:
- Needs Fixing:
	- Disklabel Associated
		- mklocale
		- disklabel
		- fdisk
	- zic /* 2.11BSD's implemented in share */
- Unimplemented:
	- rpcgen
			
### Kernel:
- Building Kernel/Arch:
	- Config:
	- Compile:
		- numerous compilation errors 

# usr/ (User & OS Libraries):
## lib:
- libc:
	- locale:
		- ctype.h rune.h & runetype.h: Need tweaking for mklocale to compile
			- various parts from the above headers need to be moved into lib/libc/locale

## libexec:

## sbin:

## share:

## tools:

## usr.bin:

## usr.lib:
- libkvm: missing functions

## usr.sbin:

# usr/sys/ (Kernel):
## conf:

## kern:
- setup: a new freeproc list. 
	- To manage proc's that are not active or zombie's.
	- Mimic 2.11BSD's original freeproc list.
- syscall proto-type declarations:
	- For when syscalls are used internally.
	- example: dup2, execve, syscall(machine), etc...
	- RLimit (Minor Issue)
		- 4.4BSD-Lite2 & 2.11BSD conflict.
		- init_main.c: plimit pointer to rlimit and user pointer to rlimit
	
## arch:
- i386/x86: (Merged under i386)
	- pcibios & pnpbios: both supported in i386/bios.c but not elsewhere

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md

## dev:
- com (Disabled): Has several comiler errors.
- usb (Disabled): Has several compiler errors that need fixing.

- Essential Driver Support:
	- Audio:
		- Check that all audio devices available are also configured	
	- Core:
		- add: parallel printer(atppc driver)
		- CARDBUS: to add
		- ISAPNP: 
			- add com
		- MCA: to add
		- PCI:
			- add agp
		- PCMCIA:
			- add com
		- SDMMC: to add
	- Disk:
		- Add: ahci & floppy
	- USB:
		- add: vhci, xhci

## fs:

## lib:
- libsa:
	- uuid: efi boot support
- libkern:
- x86emu:

## miscfs:

## net / netimp / netinet / netns:
Of Interest Todo:
- 2.11BSD's networking stack
	- To Support:
		- ipv6
		- firewall/packet filter
		- plus much more

## stand:
- boot:
	- efi:
		- Makfiles:
			- Adjust makefiles.
	- fdt:
	- uboot:
		- missing fdt support libraries
		
## ufs:
- ufs:
	- journaling
	- dirhash
- ufs211:
   	- add Extended Attributes
	- add UFML Support
- ufml:
	- adds User libs
	- improve fs support
	- add fs-based routines

## vm:

## Other:
- Memory Segmentation (Software):
	- Seperate Process Segments: text, data, stack
	- Seperate Instruction & Data Spaces
