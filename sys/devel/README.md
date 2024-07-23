## Development Folder

- All work here is proof of concept and can change without notice.

(NOTE: This folder is only temporary during initial development. It will be removed once
an offical release is made.)

### Devel Contents

- Arch: Machine dependent code to be included
  - i386: (Partially implemented in arch/i386)
	- timecounters
	- bios: pnpbios & pcibios

- Dev: Device code to be included
	- USB

- ADVVM (AdvVM): Logical Volume Manager
	- A Volume Manager for BSD.
	- Based on the ideas from Tru64 UNIX's AdvFS and incorporating concepts from Vinum and LVM2

- HTBC: A HTree Based Blockchain (WAPBL from NetBSD 5.2)
	- Provide a VFS-Layer Blockchain that can augment/improve existing filesystem/s
		- Augment BSD's Filesystems & other Filesystems including:
			- LFS: Augment the Log-Structure
			- UFS: Augment Journaling

- UFS:
	- EFS: Extent-Based Filesystem: UFS-Backend
		- Update/improve on IRIX's EFS-based extents
			- Increase Extent Max Size and 24-bit limits
		- Converting/Mapping between extents and blocks
		- Implement vnops and vfsops
	- UFS:
		- Add Journaling and Soft Updates
		- Implement Dirhash
		
- Kern & Sys:
	- Malloc: A Tertiary Buddy System Allocator (Needs a home!). (subr_tbree.c & tbtree.h)
    	- Designed to work with the existing kern_malloc.c
  	- Threading (Hybrid N:M Model): kernel & user threads
    	- Implements a new concept: Inter-Threadpool Proccess Communication (ITPC)
    	- For User Threads, goto: (devel/libuthread)
    	- implement pthreads

- VM: Todo (See: "/devel/vm")
	- Optional 
		- support for seperate Instruction & Data space
		- support for psuedo-segments (stack, code, text)

- OVL: Todo (See: "/devel/ovl")
	- Optional:
		- Support for seperate Instruction & Data space
		- Support for psuedo-segments (stack, code, text)
	- Configurable:
		- Overlay Space Size: Current Default = 10% of VM Size
      		- Number of Overlays: Current Default = 64
	- Swap support when OVL memory is full
    	- Support overlaying executeables
