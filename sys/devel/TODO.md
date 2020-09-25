- user.h:
	- short	u_uisa[16];					/* segmentation address prototypes */
	- short	u_uisd[16];					/* segmentation descriptor prototypes */
	- char	u_sep;						/* flag for I and D separation */
	- struct u_ovd						/* automatic overlay data */

FreeBSD 3.0.0:
- sys_pipe.c & pipe.h

Memory Segmentation (Hardware): CPU Registers
Memory Segmentation (Software):
Seperate Process Segments: text, data, stack
Seperate Instruction & Data Spaces



OVLSpace:
Unsolved Problems:
- physical memory (machine-dependent: arch machdep / pmap)
	- vm address physical memory with pmap intergration?
- cannot be initiated with kern_malloc (uses vm memory(ovl becomes defunct))
- seperate memory allocation: 
	- rmalloc / extent: rmalloc backed by extent allocation with ability to be extended via malloc (kernel malloc) if needed.
- Size/ space: 
	- Ovl max size (Currently): is 10% of vm space allocated (aka: VM = 4GB, OVL = 400MB)
	- memory stack layout:
		- shrink vm space when in use?
		- configurable where does it get placed in stack?
			- in-use or not: adjust the stack appropriatly?
- number of overlays:
	- Currently: is akin to 2.11BSD
	- akin to vm (w/o paging) but segmented into 2 spaces (kernel & vm)
		- OVL_OMAP = 64 (max overlay maps to allocate)
		- NKOVL = 32 (max number of kernel overlay entries)
		- NVOVL = 32 (max number of vm overlay entries)

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

- swapin & swapout feature?
	- Could prove useful: when an object is in ovlspace and needs executing
	- Used for:
		- pages
		- objects: e.g. between anon, vm & ovl
		- swapfs-like feature: (implement as a filesystem?)
			- requires vnode interaction
			- stores in swap if exceeds overlay capacity