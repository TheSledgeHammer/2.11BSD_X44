## Development: (devel folder)
- All work here is proof of concept and hence temporary.

### Devel Contents:
- HTBC: HTree Based Blockchain to augment LFS & other existing Log-Structured Filesystems (akin to Soft-updates & WAPBL)

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

- PMAP: Clustered Page Table variant, backed by a two red-black trees.

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

- VM: Updates to the VM Layer.
	- Planned: 
		- VM Overlays (OVL): A re-implementation of 2.11BSD's use of Overlays (See below: OVLSpace)
		- VM Extents: A VM extension of extents
		- Segmented VM Model (A Hybrid UVM & VM): 3 Segments:
			- VMSpace: The current VM
			- AVMSpace (Anonoymous VM): All anons, amaps & aobjects
			- OVLSpace (Overlay VM): A portion of physical memory with vm like features (See: "/devel/vm/ovl")

- MISC:
	- Crypto
	- EXT2FS: Partial port of NetBSD's ext2fs
	- UFS: Features to add
	- MPX: Multiplexors
		- A reimplementation of multiplexors from the V7 and early BSD's
		- Two concurrent versions (being considered):
			- MPX: BSD Sockets as base (Currently)
			- MPX-FS: A filesystem, with multiple possible variations
				- Implement in the VFS layer. To provide multiplexors across filesystems
				- Independent Filesystem