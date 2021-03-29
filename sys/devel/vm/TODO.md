# VM & OVL:
- Add api's and methods for (VM to OVL) & (OVL to VM) interactions

## VM:
- vm_mmap.c:
	- fill-in missing methods (sbrk, sstk & mincore)
- aobject_pager.c:
	- incomplete
- vm_swap.c/.h:
	- consider updating vm_swap with parts from uvm_swap for better anon & segment compatability
	- improve swdevt interop with swapdev
- vm_pager.c/.h:
	- keeping segment_pager seperate or as new pgops
- vm_pageout.c/.h:
	- consider updating vm_pageout with parts from uvm_pageout
	
## OVL:
- Setup OVL Minium & Maximum Size.
- OVL placement in memory stack
	- before or after vmspace?
- Add Pseudo-Segmentation Support
- Add seperate Instruction & Data spaces Support