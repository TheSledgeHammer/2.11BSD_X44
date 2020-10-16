## Development: (devel folder)
- All work here is proof of concept and can change without notice.

(NOTE: This folder is only temporary during initial development. It will be removed once
an offical release is made.)

### Devel Contents:
- Arch:
	- i386: (Partially implemented in arch/i386)
		- x86 related content. 
			- lapic, ioapic, tsc, etc..
			
- HTBC: 
	- HTree Based Blockchain to augment LFS & other existing Log-Structured Filesystems 
	- akin to Soft-updates & WAPBL

- Kern & Sys:
	- Malloc: A Tertiary Buddy System Allocator (No Plans or use cases). Originally planned as part of a larger memory allocation stack for the kernel. (Needs a home!)
	- Scheduler: A Stackable Scheduler that sits atop 2.11BSD's existing scheduler (kern_synch.c).
		- Global Scheduler: Interface/API for new schedulers
		- Hybrid EDF/CFS Scheduler
	- Threading (Hybrid N:M Model): kernel & user threads
		- Implements a new concept: Inter-Threadpool Proccess Communication (ITPC)
		- For User Threads, goto: (usr.lib/libuthread)
	- Locks: Enhancements to lock
		- Planned: Replace proc with pid in lockmgr
		- Rwlock: Introduction of a reader-writers lock
		- Witness: Partial port of FreeBSD/OpenBSD's witness

- PMAP: Clustered Page Table variant, backed by two nested red-black trees with hashing.

- UFML: LOFS based filesystem layer, combined with features from HTBC.
	- Aims to provide a Fossil + Venti inspired support to UFS, FFS, MFS & LFS.
  	- Planned Features: 
  		- Snapshots
  		- Versioning
  		- Archive
  		- Compression
  		- Encryption

- UFS211: Port of 2.11BSD's UFS Filesystem.
	- Independent of UFS

- VM: Updates to the VM Layer for Segmented Paging
	- Planned:
		- VM Extents: VM Memory Management using extent allocation (See: "/devel/vm/extents")
		- Segmented Paging VM Model (A Hybrid of UVM & VM):
			- VMSpace (aka VM): The current VM 
			- AVMSpace (aka AVM): All anons, amaps & aobjects (See: "/devel/vm/avm")
			- OVLSpace (aka OVL): A portion of physical memory with vm like features (See: "/devel/vm/ovl")
					- A re-implementation of 2.11BSD's use of Overlays
					- With support for VM & Kernel