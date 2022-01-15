A General todo list. Don't hesitate to add to this list. :)

# TODO:
## Compiler:
### Tools:
- Needs Fixing:
	- Disklabel Associated
		- mklocale
		- disklabel
		- fdisk
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
- libkvm: missing functions

## usr.sbin:

# usr/sys/ (Kernel):
## conf:

## kern:

## arch:
- i386/x86: (Merged under i386)
	- pcibios & pnpbios: both supported in i386/bios.c but not elsewhere

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md

## dev:
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
	- Disk:
		- Add: ahci & floppy
	- USB:
		- add: vhci, xhci
		- update existing drivers

## fs:
- msdosfs:
	- needs revising: bugs abound 
	- vops/vfsops out of sync with kernel vfs

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
