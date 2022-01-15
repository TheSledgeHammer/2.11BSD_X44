# VM & OVL:
- Add api's and methods for (VM to OVL) & (OVL to VM) interactions
	- Possible use cases:
		- Caching of Objects
		- Object Shadows
		- Collapsing & Coalescing Objects
		- Copying Objects, Segments &/or Pages
		- Memory Stack/Space Resizing
		
## VM:
- vm_mmap.c:
	- fill-in missing methods (sbrk, sstk & mincore)
- vm_proc.c: temporary placeholder
	- merge relavent methods into vm_mmap.c
	- merge relavent methods into vm_drum.c
	- merge relavent methods into vm_swap.c
- vm_swap.c/.h & vm_swapdrum.c:
	- improve swdevt interop with swapdev:
	- Ties into vm_drum, vm_stack & vm_text
	
## OVL:
- Setup OVL Minium & Maximum Size.
- OVL placement in memory stack
	- before or after vmspace?
- Add Pseudo-Segmentation Support
- Add seperate Instruction & Data spaces Support
		
/*
 * Process Segmentation:
 * Order of Change:
 * Process/Kernel:
 * 1. Process requests a change is size of segment regions
 * 2. Run X kernel call (vcmd (exec) most likely)
 * 3. Runs through vmspace execution
 * 4. return result
 *
 * VMSpace execution:
 * 1. vm_segment copies old segments to ovlspace.
 * 2. vm_segment resizes segments, while retaining original segments in ovlspace
 * 3. Determine if segment resize was a success
 * 3a. Yes (successful). vm_segment notifies ovlspace to delete original segments.
 * 3b. No (unsuccessful). vm_segments notifies ovlspace to copy original segments back and before removing from ovlspace
 */