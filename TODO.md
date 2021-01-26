A General todo list. Don't hesitate to add to this list. :)

# TODO:
## General:
- Compiler
- Makefiles
- Bug Fixes &/or missing critical content
- Cross-Compatability with 4.4BSD: Following routines are used:
	- e.g. groupmember & suser
	- kern_prot.c ???

# usr/ (User & OS Libraries):
## lib:
- libkvm
		
## libexec:
		
# usr/sys/ (Kernel):
## conf:

## kern:
- kern_prot/2.c: credentials- ucred & pcred structs
	
## arch:
- i386/x86: (Merged under i386)
	- Move common code to /dev in following: spkr.c, spkr.h, spkrreg.h, timerreg.h

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md

- HTBC:
	- Modify structures under htbc layout and htbc htree.
		- Inodes & HTree:
			- Specifically struct htbc_hc_blocklist, htbc_hc_inodelist & htbc_ino
			- These are double ups on structures present in htbc htree and should be merged
		- Transactions/Blockchain:
			- Merge htbc_entry & htbc_hchain
			- Do the same thing. 
		- Journalling & Logs:
			- integrate later
			- Merge struct htbc_hi_fs & htbc_hc_header

- dev/kbd: current implementation: ((WILL NOT WORK AS IS!!!) Would require the entire device layout to be re-implemented around FreeBSD)
	- no virtual keyboard in kernel (deprecated)
	- resource maps: change from FreeBSD/ DragonflyBSD's to NetBSD style
	- keyboard drivers & config driver (cfdriver) are not correct. example: genkbd.c
		- current cfdriver/data structure: 
			- cannot manage multiple config drivers through a single driver interface as implemented
			by FreeBSD/ DragonflyBSD keyboard.
		- each cfdriver/data structure:
			- Needs a device reference and the following information
				- devs (if applicable), name, match, attach, flags, and size
	- kbd changes:
		- remove need for the in-kernel virtual keyboard driver (used with FreeBSD's sysinit)
		- provide a generic keyboard layer with routines that can manage multiple.
		OR
		- At minimum a machine-independent keyboard driver with PS/2 compatability
		- cfdriver is only used with the driver not the generic keyboard layer
	
## dev:
- Essential Driver Support:
	- usb: 								Work in progress
	- video:								Work in progress
	- mouse/keyboard:						Work in progress
- Add: Common Speaker code from i386
- Fix:
	- eisa: Missing tidbits.
	- pci: Missing tidbits.

## lib:
	
## net / netimp / netinet / netns:
Of Interest Todo:
- 2.11BSD's networking stack
	- No Support for ipv6, firewall/packet filter plus much more

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
- ufs/ffs:
	- dirhash
	- ufs2/ffs2 support: Work in Progress
		- Fix ffs/fs.h: To account for ufs1 & ufs2
			- #define blksize(fs, ip, lbn) 
	- journaling
	
## vfs:

## vm:

## Other:
- Memory Segmentation (Hardware): CPU Registers
- Memory Segmentation (Software):
- Seperate Process Segments: text, data, stack
- Seperate Instruction & Data Spaces
