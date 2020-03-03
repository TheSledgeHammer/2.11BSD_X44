# 2.11BSDx86 (TBA)

2.11BSDx86 is 2.11BSD with the 4.3BSD styled VM space and inodes, replaced with 4.4BSD-Lite 2 VM space and vnodes. While
retaining 2.11BSD's kernel and userspace. 2.11BSDx86 adopts the 4.4BSD & later BSD's (i.e. FreeBSD, NetBSD, OpenBSD & DragonflyBSD)
approach of having a clearly defined architechture dependent code and architechture independent code, allowing for easier portability.

## Project Goals:
- Maintain a smaller footprint then current BSD's
    - Ideally 155 (2.11BSD's) up to 200 System Calls.
- Maintain Traditional BSD Style Approaches while also using current approaches

## Current Project Tasks:
### Short-Term:
- Memory Management:
    - Zoned Tertiary Buddy Allocator
- Stackable CPU Scheduler: Running on top 2.11BSD's
    - Hybrid EDF & CFS Scheduler
- Improve Multitasking:
    - Hybrid N:M Threads:
        - Kernel & User
        - Threadpool
    - Deadlocking
    - Mutual Exclusion
    - Readers-Writer Lock
- Networking:
    - Firewall/ Packet Filter (Any or all): NPF, PF, ipf, ipfw
    - IPv6
- Filesystem (UFS, FFS & LFS):
    - Dirhash
    - Wapbl
    - Extended Attributes

### Long-Term:
- Migrate to a UVM-like VM.
- Improve Kernel's Dual Control (Proc & User)
- Improve Filesystem Support
- Improve UFS, FFS & LFS
- 2.11BSD's Kernel Overlays:
    - Kernel &/or VM Space
- Symetric Multi-Processing (SMP)
- AMD64/ x86_64 (64-bit)
- Package Management