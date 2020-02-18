# 2.11BSDx86 (TBA)

2.11BSDx86 is 2.11BSD with the 4.3BSD styled VM space and inodes replaced with 4.4BSD-lite 2 VM space and vnodes. While
retaining 2.11BSD's kernel and userspace. 2.11BSDx86 adopts the 4.4BSD & later BSD's (i.e. FreeBSD, NetBSD, OpenBSD & DragonflyBSD)
approach of having a clearly defined architechture dependent code and architechture independent code, allowing for easier portability.

## Current Project Tasks:
- Memory Management:
    - Zoned Tertiary Buddy Allocator
- Stackable CPU Scheduler: Running on top 2.11BSD's
    - Hybrid EDF & CFS Scheduler
- Improve Multitasking:
    - Hybrid N:M Threads
    - Deadlocking
    - Mutual Exclusion
    - Readers-Writer Lock
- Add Filesystem Support:
    - Currently Supports: UFS, LFS & FFS
- Networking:
    - Firewall/ Packet Filter (Any or all): NPF, PF, ipf, ipfw
    - IPv6
- Symetric Multi-Processing (SMP)
- AMD64/ x86_64 (64-bit)
- Package Management