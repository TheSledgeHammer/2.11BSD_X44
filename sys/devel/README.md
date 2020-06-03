Development Folder: Can be split into two. 
	- New Content: code in development. 
	- Existing Content: code to be added into kernel

New Content:
- HTBC: HTree Blockchain
	- A Versioning, Snapshot, Cache augmentation for LFS and other Log-Structured Filesystems. 
	- Using a blockchain styled structure 
	- Implemented in both LFS and the VFS Layer 
- Memory:
	- An general memory allocator with several layers/ stack. 
	- Idea: Top(Interface) to Bottom(Block Allocation) Layers: Slabs, Pools, Buckets, Tertiary Buddy 
- MPX: Multiplexors
	- A reimplementation of multiplexors from the V7 and early BSD's
	- Two concurrent versions:
		- MPX: Based on bsd sockets
		- MPX-FS: A filesystem, with multiple possible variations
			- Implement in the VFS layer. To provide a multiplexors across filesystems 
			- Independent FS like fdesc, fifo, etc 
- Multitasking:
	- Threads
	- Threadpools
	- Deadlocking: Mutexes, Reader-Writer Lock
- Pmap:
	- Machine-independent api for Clustered Page Tables variation.
	- i386 arch partial pmap implementation
- Scheduler: Stacked Scheduler
	- Kernel feeds EDF which feeds the CFS. Circuler Queue
	- Earliest Deadline First Priorities (Plan 9's EDFI Inspired)
	- Completely Fair Scheduler
- UFML:
	- UFML is an abbrievation of the filesystems it's designed for (UFS + FFS + MFS + LFS)  
	- A content based filesystem/layer inspired by Plan 9's Fossil & Venti Filesystem
	- Provides a modular unified layer for archiving, snapshots and compression across UFS, FFS, MFS & LFS
- UFS211:
	- 2.11BSD's UFS. (Seperate from UFS)
	- TODO: 
		- vnode integration
		- remove pdp memory refrences 
	
Existing Content:
- Misc:
	- Crypto: Camellia, Murmur3 & Twofish
- UFS: Yet to be implemented
	- UFS WAPBL
	- VFS WAPBL
	- dirhash
	- extended attributes
