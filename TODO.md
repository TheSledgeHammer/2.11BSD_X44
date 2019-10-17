In Kernel:
PDP-11 Segmentation Pointers: Most internal kernel reference have been removed or replaced.
- Remove:
- Create:
- Add:

Hardware Abstraction Layer:
- Memory Abstraction (Physical Memory, Virtual Memory):


From Plan 9:
- Pool.c/ Pool.h: Uses tree structure with areanas (like slabs). 
- Combined with a malloc, calloc and alloc.

