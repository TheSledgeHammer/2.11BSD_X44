## Development: ("sys/devel" folder)
- All work here is proof of concept and can change without notice.

(NOTE: This folder is only temporary during initial development. It will be removed once
an offical release is made.)

### Devel Contents:
- Arch:
	- i386: (Partially implemented in arch/i386)
		- x86 related content. (To be implemented)
			- lapic, ioapic, tsc, intr, pmap etc..
			
- ADVVM (AdvVM): Logical Volume Manager
	- A Volume Manager for BSD.
	- Based on the ideas from Tru64 UNIX's AdvFS and incorporating concepts from Vinum and LVM2
				
- HTBC: A HTree Based Blockchain
	- Intended to augment BSD's LFS & other Log-Structured Filesystems 
	- provide features of a blockchain to the VFS

- Kern & Sys:
	- Malloc: A Tertiary Buddy System Allocator (Needs a home!). (subr_tbree.c & tbtree.h)
		- Implemented to work with the existing kern_malloc.c
		- Example in following folder "/devel/vm/ovl/kern_overlay.c"
	- Scheduler: A Stackable Scheduler that sits atop 2.11BSD's existing scheduler (kern_synch.c).
		- Global Scheduler: Interface/API for other schedulers
		- Hybrid EDF/CFS Scheduler
	- Threading (Hybrid N:M Model): kernel & user threads
		- Implements a new concept: Inter-Threadpool Proccess Communication (ITPC)
		- For User Threads, goto: (usr.lib/libuthread)

- UFML: A Fossil + Venti inspired filesystem layer 
	- LOFS based, intended to support to UFS, FFS, MFS & LFS.
  	- Planned Features: (when combined with the HTBC)
  		- Snapshots
  		- Versioning
  		- Archive
  		- Compression
  		- Encryption

- UFS211: A port of 2.11BSD's UFS Filesystem.
	- Vnode integration
	- Planned:
		- UFML Support
		- Extended Attributes

- VM: Updates & Changes (See: "/devel/vm")
	- Planned:
		- Virtual Segments: Logical Address
			- Improve internal VM operations.
				- Most noteably when used in conjunction with overlays (see below)
			- Planned:
				- Optional support for seperate Instruction & Data space
				- Optional support for psuedo-segments (stack, code, text)
		- Overlay Space: A re-implementation of Overlays from 2.11BSD (See: "/devel/vm/ovl")
			- OVLSpace: A portion of physical memory with vm-like features 
			- Supports vm objects, pages & segments.
			- Planned:
				- Optional:
					- Support for seperate Instruction & Data space
					- Support for psuedo-segments (stack, code, text)
				- Configurable:
					- Overlay Space Size: Current Default = 10% of VM Size
					- Number of Overlays: Current Default = 64
				- Swap support when OVL memory is full
				- Support overlaying executeables