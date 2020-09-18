- user.h:
	- short	u_uisa[16];					/* segmentation address prototypes */
	- short	u_uisd[16];					/* segmentation descriptor prototypes */
	- char	u_sep;						/* flag for I and D separation */
	- struct u_ovd						/* automatic overlay data */

NetBSD 5.0.2: Threads & Multitasking
- kern_proc.c
- kern_lwp.c
- kern_rwlock.c
- kern_runq.c
- subr_threadpool.c
- subr_workqueue.c

FreeBSD 3.0.0:
- sys_pipe.c & pipe.h

Memory Segmentation (Hardware): CPU Registers
Memory Segmentation (Software):
Seperate Process Segments: text, data, stack
Seperate Instruction & Data Spaces


OVLSpace:
Unsolved Problems:
- cannot be initiated with kern_malloc (uses vm memory)
- seperate memory allocation: 
	- extents?
		- provides useful integration with kern_malloc if needed
		- test if sub-region
	- rmalloc?
		- independent but inefficient: memory fragmentation may become an issue
		- required before being used: improvements & possibly intergration with kern_malloc/ extents
- Size/ space: 
	- Ovl max size (Currently): is 10% of vm space allocated (aka: VM = 4GB, OVL = 400MB)
	- memory stack layout:
		- shrink vm space when in use?
		- configurable where does it get placed in stack?
			- in-use or not: adjust the stack appropriatly?
- number of overlays:
	- if extents: allow for sub-regions?
		- similar objects/blocks can be allocated within the same extent
	- Currently: is akin to 2.11BSD 
		- NOVL = 32 (max number of overlays)
		- NOVLSR = (NOVL/2) (max number extent sub-regions)
		- NOVLPSR = NOVLSR 	(max number of overlays in a subregion)

TODO:
- missing underlying storage: 
	- map interaction for objects
	- allocation of map & objects
	- store of objects? (vm_pager/vm_page equivalent)
	
TO FIX:
- ovl_map:
	- ovl_map_create
	- ovl_map_entry_create
	- ovl_map_entry_dispose
	- ovl_map_entry_delete
	- test for vm_map & or vm_object
- swapin & swapout feature?
	- Could prove useful: when an object is in ovlspace and needs executing
	- Used for:
		- pages
		- objects: e.g. between anon, vm & ovl
		- swapfs-like feature: (implement as a filesystem?)
			- requires vnode interaction
			- stores in swap if exceeds overlay capacity