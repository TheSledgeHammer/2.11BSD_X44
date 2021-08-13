## Development Folder

- All work here is proof of concept and can change without notice.

(NOTE: This folder is only temporary during initial development. It will be removed once
an offical release is made.)

### Devel Contents

- Arch:
  - i386: (Partially implemented in arch/i386)
	- smp/multi-cpu:
		- machine-independent code: 90% complete
			- smp-related methods for cpu
		- machine-dependent code: 75% complete
			- boot: considering FreeBSD's mpboot.s
			- smp: alloction to assign interrupts to CPUs
			- tsc: missing struct timecounter

- ADVVM (AdvVM): Logical Volume Manager
  - A Volume Manager for BSD.
  - Based on the ideas from Tru64 UNIX's AdvFS and incorporating concepts from Vinum and LVM2

- HTBC: A HTree Based Blockchain (WAPBL from NetBSD 5.2)
  - Provide a VFS-Layer Blockchain that can augment/improve existing filesystem/s
    - Augment BSD's Filesystems & other Filesystems including:
      - LFS: Augment the Log-Structure
      - UFS: Augment Journaling

- MPX: A reimplementation of the Multiplexor.
 Changes from Original:
  - Replaces the pipe equivalent in FreeBSD, NetBSD, OpenBSD & DragonflyBSD
  - Does not implement or use the line output.
  - Interacts with the vfs layer rather than the filesystem.
  - Meant to be interoperate with sockets

- Kern & Sys:
  - Malloc: A Tertiary Buddy System Allocator (Needs a home!). (subr_tbree.c & tbtree.h)
    - Designed to work with the existing kern_malloc.c
    - Example of potential implementation in "/devel/kern/kern_malloc2.c"
  - Scheduler: A Stackable Scheduler that sits atop 2.11BSD's existing scheduler (kern_synch.c).
    - Global Scheduler: Interface/API for other schedulers
    - Hybrid EDF/CFS Scheduler
  - Threading (Hybrid N:M Model): kernel & user threads
    - Implements a new concept: Inter-Threadpool Proccess Communication (ITPC)
    - For User Threads, goto: (devel/libuthread)
      - implement pthreads

- UFML: A Fossil + Venti inspired filesystem layer
  - LOFS based, intended to support to UFS, FFS, MFS & LFS.
  - Planned Features:
    - Snapshots
    - Versioning
    - Archive
    - Compression
    - Encryption

- UFS211: A port of 2.11BSD's UFS Filesystem. (See: "/ufs/ufs211")
  - Vnode integration
  - Planned:
    - UFML Support
    - Extended Attributes

- VM: Updates & Changes (See: "/devel/vm")
- Planned:
  - Ovl and Vm interactions
    - Allowing vm to make use of the ovl
  - Anonymous virtual memory support: (see: "devel/vm/uvm")
    - Ports the anonymous virtual memory from NetBSD's UVM
      - uvm_anon: aka vm_anon: working as is. Minor changes needed
      - uvm_amap: aka vm_amap: working as is. Minor changes needed
      - uvm_aobj: aka vm_aobject: following changes needed:
        - uao_pager: seperate to uao_pager
        - remove duplicate vm_object references: e.g. uao_reference
        - uao_create: deprecated.
          - vm object is already created.  
          - Only needed for initializing an aobject (as is), which invokes the point below with swap.
        - Interactions with swap.
          - Either:
       a) modify swap to make use of these changes.
       b) modify aobject

  - Virtual Segments: Logical Address
    - Improve internal VM operations.
      - Most noteably when used in conjunction with overlays (see below)
    - Planned:
      - Optional support for seperate Instruction & Data space
      - Optional support for psuedo-segments (stack, code, text)

- OVL: Overlay Space: A re-implementation of Overlays from 2.11BSD (See: "/devel/vm/ovl")
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
