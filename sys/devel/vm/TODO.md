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
- kern_slab:
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