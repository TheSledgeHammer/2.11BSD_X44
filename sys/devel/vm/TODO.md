# VM & OVL:
- Add api's and methods for (VM to OVL) & (OVL to VM) interactions

## VM:
- vm_mmap.c:
	- fill-in missing methods (sbrk, sstk & mincore)
- vm_swap & vm_swapdrum.c/.h:
	- consider updating vm_swap with parts from uvm_swap for better anon & segment compatability
	- improve swdevt interop with swapdev:
	- Ties into vm_drum, vm_proc, vm_stack & vm_text
	
## OVL:
- Setup OVL Minium & Maximum Size.
- OVL placement in memory stack
	- before or after vmspace?
- Add Pseudo-Segmentation Support
- Add seperate Instruction & Data spaces Support


## MISC:
- aobject_pager & segment_pager:
	- Keep independent or create new pgops for aobjects & segments
	
- vm_slab:
	- Backends:
		- Extents: pros & cons?
		- Kern_Malloc (Current):
			- Slabs could enable some of the following:
				- better bucket utilization: slabs provide information on the current
				state of a bucket (full, empty, etc). 
				- Better decision making: changing allocation algorithms from a best-fit to a first-fit
				- improved distribution: from above
	- Location?
		- For In Kernel:
			- currently offers no benefit in vm. All inner workings utilize the buckets in kern_malloc.
			Only being executed from within the kern_malloc routines. 
		- For in VM:
			- A FreeBSD UMA style implemenation.
		
- vm_stack/vm_text (Pseudo-Segmentation):
	- vmspace access
	- Memory Management:
		- rmap backed by an extent allocator
		
		