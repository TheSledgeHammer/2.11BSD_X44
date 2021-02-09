# VM & OVL:
- Add api's and methods for (VM to OVL) & (OVL to VM) interactions

## VM:
- vm_mmap.c:
	- fill-in missing methods (sbrk, sstk & mincore)
- uvm_aobject.c

## OVL:
- Setup OVL Minium & Maximum Size.
- OVL placement in memory stack
	- before or after vmspace?
- Add Pseudo-Segmentation Support
- Add seperate Instruction & Data spaces Support