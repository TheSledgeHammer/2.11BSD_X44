LOFS:
	- Fix to match current vfs
UFML:
	- Fix to match current vfs
	- Fix-up ufml_subr.c
- user.h:
	- short	u_uisa[16];					/* segmentation address prototypes */
	- short	u_uisd[16];					/* segmentation descriptor prototypes */
	- char	u_sep;						/* flag for I and D separation */
	- struct u_ovd						/* automatic overlay data */
	
kern:
	- ksyms: dev: partly complete (open, read, attach, & close)
		- move rest to dev

NetBSD 5.0.2: Threads & Multitasking
- kobj: Loading objects in filesystem
- wapbl
- kern_proc.c
- kern_lwp.c
- kern_rwlock.c
- kern_runq.c
- subr_workqueue.c

FreeBSD 3.0.0:
- sys_pipe.c & pipe.h

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
