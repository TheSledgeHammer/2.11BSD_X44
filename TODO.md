In Kernel:
PDP-11 Segmentation Pointers: Most internal kernel reference have been removed or replaced.
- Remove:
- Create:
- Add:

Hardware Abstraction Layer:
- Memory Abstraction (Physical Memory, Virtual Memory):


From Plan 9:
- Pool.c, Pool.h, malloc.c, ucalloc.c, alloc.c, xalloc.c: Uses a tree structure. 
- edf.c, edf.h

/*
Refer to:
Plan 9:
 plan9/sys/src/9/pc/mem.h
 plan9/sys/src/9/pc/mmu.c 		(initiates memory pool)
 plan9/sys/src/9/port/alloc.c
 plan9/sys/src/libc/port/malloc.c 
 plan9/sys/src/9/port/ucalloc.c
 plan9/sys/src/9/port/xalloc.c 
*/