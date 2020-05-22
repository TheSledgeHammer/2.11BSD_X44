TODO:
- Build Toolchain (Cross-Compile): NetBSD's
	- Attempt build on NetBSD (Can't build on Linux without a Toolchain)
	- Makefiles
- Merge KASSERT CTASSERT into ASSERT

- boot:
	- common:
		- isapnp.c/.h
		- pnp.c 
	- efi
	- usb
	- i386:
		- gptboot
		- isoboot
		- loader:
			- Makefile
			- Makefile.depend
		- pmbr
		- pxeldr

- arch:
	- i386:
		- Makefile
		- dev/isa/isadmavar.h
		- bus_machdep.c
		- consinit.c
		- machdep.c: cpu_reboot, cpu_reset
		- vm_machdep.c: u->u_procp->p_p0br: (no reference in 4.4BSD-Lite2)
			- 4.3BSD Reno/ 4.4BSD Remanent: once in struct proc. Obsolete?? 

- ufs:
	- ext2fs (NetBSD)
	- ufs:
		- ufs1 & ufs2?
		- dirhash
		- extattr
		- wapbl

Kern:
- kern_physio.c: Incomplete: getphysbuf, putphysbuf (NetBSD 1.3)
- if INET: remanents of 2.11BSD's networking stack overlay (keep in place for now)
- syscallargs: To be fixed: temporary compat with 4.4BSD-Lite2

- user.h: 
	- short	u_uisa[16];					/* segmentation address prototypes */
	- short	u_uisd[16];					/* segmentation descriptor prototypes */
	- char	u_sep;						/* flag for I and D separation */
	- struct u_ovd						/* automatic overlay data */
	
NetBSD 5.0.2: Threads & Multitasking
- kobj: Loading objects in filesystem
- wapbl
- kern_proc.c
- kern_lwp.c
- kern_rwlock.c
- kern_runq.c 
- subr_workqueue.c

- devel/multitasking
	- kernthreads: (NetBSD 1.4/1.5)
		- kern_kthread.c: kthread_create, exit, create_deferred, run_deferred_queue
		- init_main: starting of threads
- devel/vm:
	- vm_seg/vm_extent (must be compat with pmap, pde & pte)
		- NOTE: Overlays: need to be allocated physical for max benefit in vmspace
		- map into pmap: pmap_init or vm_mem_init
		- segmented space can be initiated in one of the above 
		OR
		- pmap_segment: a method to allocate segments in either physical or virtual
			- called from pmap_init 
			OR 
			- integrated into pmap_init and called in vm_mem_init 		
	- vm_map: (uvm) (NetBSD 1.4/1.5) 
		- vm_unmap
		- vm_unmap_remove
		- vm_map_modflags
		- vmspace_share
		- vmspace_unshare			

Memory Segmentation (Hardware): CPU Registers
Memory Segmentation (Software):
Seperate Process Segments: text, data, stack
Seperate Instruction & Data Spaces

