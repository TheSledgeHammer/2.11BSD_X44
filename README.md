# 2.11BSD Kernel
- 2.11BSD Kernel Port to i386
- Most code needed or related to the i386 arch is ported from FreeBSD 2.0 and 4.4BSD-Lite2 or modified from those previously mentioned sources.

#### Notable Changes:
- Directory structure more closely follows that of FreeBSD & Later BSD4 releases including architechture ports
- Most machine-dependent references (i.e. pdp-11) have been removed or replaced as neccessary for easier portablity.
- Virtual Memory is a mix of both 2.11BSD sub_rmap and 4.4BSD-lite2 kern_malloc. With the 2.11BSD sub_rmap serving as macro memory allocator & the 4.4BSD-lite2 kern_malloc providing finer grain control of that allocated space (In Theory).

### Additions Folder (usr/src/sys/additions):
All content in this folder is temporary before being moved to a permenant location

FreeBSD 2.0 Code:
- imgact.h
- imgact_aout.h
- imgact_aout.c
- link_aout.h
- link_elf.h
- mman.h
- nlist_aout.h
- resourcevar.h
- sysent.h
- types2.h (additional definitions etc needed for above)

BSD4.4-lite2 Code:
- malloc.h
- kern_malloc.c