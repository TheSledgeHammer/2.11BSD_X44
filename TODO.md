In Kernel:
PDP-11 Segmentation Pointers:
- Remove:
    - data
    - text
    - stack
    - u (user)
- Modify:
    - User.h: Currently contains pointers to above pdp-11 segmentation.
    - seg.h: In PDP: KISA6, KDSA6
- Create:
Hardware Abstraction Layer:
- Memory Abstraction (Physical Memory, Virtual Memory):
    - Provided for both Kernel & User.
    - Pointers to said memory address space.
    - Above pointers are defined by cpu arch (i.e. x86, vax, arm)
    - Possible if kernel knows the cpu arch's Memory Address Boundaries (Likely what NetBSD does)
    - Page Tables will complicate this (not all arches provide the ability for page tables)

Look at 3BSD & 4.3BSD for ideas in implementing the above.

TODO:
- Add:
    - vm_unix.h: Doesn't rely on vm.h etc...