Development Folder: Can be split into two. 
	- New Content: code in development. 
	- Existing Content: code to be added into kernel

New Content: 
- Memory:
	- An general memory allocator with several layers. 
	- Top(Interface) to Bottom(Block Allocation) Layers: Slabs, Pools, Buckets, Tertiary Buddy 
- MPX: Multiplexors (Unix V7)
	- A reimplementation of 2.11BSD's multiplexors
	- Could merge it with UFS211 (FS Multiplexor rebase/extension...! )
		- UFS211 contains most key references needed.
- Multitasking:
	- Kernel & User Threads
	- Threadpools (kernel & userspace)
	- DeadLocks: Mutexes, Readers-Writer Lock
- Pmap:
	- Machine-independent api for Clustered Page Tables variation.
	- i386 arch partial pmap implementation
- Scheduler:
	- Kernel feeds EDF which feeds the CFS. Circuler Queue
	- Earliest Deadline First Priorities (Plan 9's EDFI Inspired)
	- Completely Fair Scheduler
- UFML:
	- UFML is an abbrievation of (UFS + FFS + MFS + LFS);  
	- A content based filesystem designed for UFS, FFS, MFS & LFS (Plan 9's Fossil & Venti Inspired)
- UFS211:
	- 2.11BSD's UFS. (Seperate from current UFS)
	- TODO: 
		- vnode integration
		- remove pdp memory refrences 
- VFS:
	- replacement for vnode_if
	
Existing Content:
- Misc:
	- Crypto: Camellia, Murmur3 & Twofish
	- Hashed Red-Black Tree
- UFS:
	- UFS WAPBL
	- VFS WAPBL
	- dirhash
	- extended attributes
