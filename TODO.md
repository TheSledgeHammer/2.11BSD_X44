A General todo list. Don't hesitate to add to this list. :)

# TODO:
## Compiler:
### Tools:
- Needs Fixing:
	- Disklabel Associated
		- disklabel
		- fdisk
- Unimplemented:
	- rpcgen
			
### Kernel:
- Building Kernel/Arch:
	- Compile:
		- numerous compilation errors 

# usr/ (User & OS Libraries):
## lib:

## libexec:

## sbin:

## share:

## tools:

## stand:
- boot:
	- setup configuration settings for elf & non-elf based kernels
	- add gfx_fb
	- efi:
		- refactor
		- add fdt & gptboot directory
	- uboot:
		- refactor

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
- RLimit (Minor Issue)
	- 4.4BSD-Lite2 & 2.11BSD conflict.
	- init_main.c: plimit pointer to rlimit and user pointer to rlimit
	
## arch:
- i386/x86: (Merged under i386)
	- setup kernel compiler support for bootloader
		- see conf/kern.post.mk for initial port from FreeBSD
	- pcibios & pnpbios: both supported in i386/bios.c but not elsewhere

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md

## dev:
- com (Disabled): Has several compiler errors.
- usb (Disabled): Has several compiler errors.

- Essential Driver Support:
	- Audio:
		- Check that all audio devices available are also configured	
	- Core:
		- CARDBUS: to add
		- ISAPNP: 
			- add com
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
