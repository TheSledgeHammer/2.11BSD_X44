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
-
	
## arch:
- i386/x86: (Merged under i386)

## devel: (planned)
- Code planned for future integration
- update copyright headers
- See devel folder: README.md

- HTBC:
	- Modify structures under htbc layout and htbc htree.
		- htbc_transaction_len/htbc_transaction_inodes_len:
			- use to compute the blockchain (i.e. number of blocks is length of blockchain)
			- insert an inode/htree into each block
			- htbc_ino (should be part of blockchain)
		- Inodes & HTree:
			- Specifically struct htbc_hc_blocklist, htbc_hc_inodelist
			- These are double ups on structures present in htbc htree and should be merged
			- htbc_inodetrk_init, htbc_inodetrk_free, htbc_inodetrk_get, 
			- htbc_register_inode, htbc_unregister_inode
		- Transactions/Blockchain:
			- Merge htbc_entry & htbc_hchain
			- Do the same thing. 
		- Journalling & Logs:
			- integrate later
			- Merge struct htbc_hi_fs & htbc_hc_header
	
## dev:
- Essential Driver Support:
	- usb: 								Work in progress
	- video:								Work in progress
	- mouse/keyboard:						Work in progress
- Fix:
	- video (kqfilter)
	
- Syscons:	
	- kbd:
		- kbd changes:
			- remove need for the in-kernel virtual keyboard driver (used with FreeBSD's sysinit)
			- provide a generic keyboard layer with routines that can manage multiple.
			OR
			- At minimum a machine-independent keyboard driver with PS/2 compatability
			- cfdriver is only used with the driver not the generic keyboard layer

## fs:

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

## vm:

## Other:
- Memory Segmentation (Software):
	- Seperate Process Segments: text, data, stack
	- Seperate Instruction & Data Spaces
