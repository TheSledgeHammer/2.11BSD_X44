# 2.11BSDx86 (TBA)
2.11BSD is unique in BSD's history, still recieving patches to it's codebase up until the last 10 years. As well as being the only BSD to receive patches that were backported from numerous versions of 4.3BSD.
2.11BSDx86 is 2.11BSD that continues with that tradition. Replacing the 4.3BSD styled VM space and inodes with 4.4BSD-Lite2 VM space and vnodes. While retaining 2.11BSD's kernel and userspace. 2.11BSDx86 adopts the 4.4BSD & later BSD's (i.e. FreeBSD, NetBSD, OpenBSD & DragonflyBSD) approach of having a clearly defined architechture dependent code and architechture independent code, allowing for easier portability.

## Project Goals:
- Maintain a small footprint
    - Ideally 155 (2.11BSD's) up to 200 System Calls
- Maintain Traditional BSD Style Approaches while also using current approaches
- Clean code

## Current Project Tasks:
### Short-Term:
- Memory Management:
    - Zoned Tertiary Buddy Allocator
    - Pools
    - Slab Allocator
- Stackable CPU Scheduler: Running on top of 2.11BSD's
    - Hybrid EDF & CFS Scheduler
- Improve Multitasking:
    - Hybrid N:M Threads:
        - Kernel & User
        - Threadpools
    - Mutual Exclusion
    - Readers-Writer Lock
- Networking:
    - Firewall/ Packet Filter (Any or all): NPF, PF, ipf, ipfw
    - IPv6
- Filesystem (UFS, FFS & LFS):

### Long-Term:
- Migrate to a UVM-like VM.
- Improve Kernel's Dual Control (Proc & User)
- Improve Filesystem Support
- Improve UFS, FFS & LFS
    - Dirhash
    - WAPBL
    - Extended Attributes
- 2.11BSD's Kernel Overlays:
    - Kernel &/or VM Space
- Kernel Modules
- Symetric Multi-Processing (SMP)
- AMD64/ x86_64 (64-bit)
- Package Management