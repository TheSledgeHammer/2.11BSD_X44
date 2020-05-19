# 2.11BSD-RenoX44
2.11BSD is unique in BSD's history, still recieving patches to it's codebase up until the last 10 years. As well as being the only BSD to receive patches that were backported from numerous versions of 4.3BSD.

2.11BSD-RenoX44 is 2.11BSD that continues with that tradition. Replacing the 4.3BSD styled VM space and inodes with 4.4BSD-Lite2 VM space and vnodes. While retaining 2.11BSD's kernel and userspace. 2.11BSDx86 adopts the 4.4BSD & later BSD's (i.e. FreeBSD, NetBSD, OpenBSD & DragonflyBSD) approach of having a clearly defined architechture dependent code and architechture independent code, allowing for easier portability.

## Project Goals:
- Maintain Traditional BSD Style Approaches while also using current approaches
- Clean code

## Current Project Tasks:
### Short-Term:
- Memory Management:
    - Tertiary Buddy Allocator
    - Pools
    - Slab Allocator
- Stackable CPU Scheduler: Running on top of 2.11BSD's
    - Hybrid EDF & CFS Scheduler
- Improve Multitasking:
    - Hybrid N:M Threads:
        - Kernel & User
        - Threadpools (User & Kernel Space)
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
- Overlays (2.11BSD):
    - Kernel
    - VM Space
- Kernel Modules
- Symetric Multi-Processing (SMP)
- AMD64/ x86_64 (64-bit)
- Package Management

## Contribution:
- Anyone is welcome to contribute. 
- Your code should ideally fit the following guidelines
- Licencing: Most permissive licences. 
	- Preferred: The 3-Clause BSD Licence.
- These rules in place to make it pleasant for everyone.
	- It is highly recomended that you adhere to the following rules. 
	- Failing to do so will prevent your contribution from being accepted. 

### Contribution Guidelines:
1. Clean Code.
2. Contain a Licence Header. Clearly stating year, author and the licence. 

### Contribution Rules:
1. No profanity
2. No abuse of others
	- Constructive criticsm is welcome