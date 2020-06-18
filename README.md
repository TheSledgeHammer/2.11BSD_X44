# 2.11BSD-X44
2.11BSD is unique in BSD's history, still receiving patches to it's codebase up until the last 10 years. As well as being the only BSD to receive patches that were backported from numerous versions of 4.3BSD.

2.11BSD-X44 is 2.11BSD that continues with that tradition. Replacing the 4.3BSD styled VM space and inodes with 4.4BSD-Lite2 VM space and vnodes. While retaining 2.11BSD's kernel and user space. 2.11BSD-X44 adopts the 4.4BSD & later BSD's (i.e. FreeBSD, NetBSD, OpenBSD & DragonflyBSD) approach of having a clearly defined architecture dependent code and architecture independent code, allowing for easier portability.

## Project Goals:
- Maintain Traditional BSD Style Approaches while also using current approaches
- Clean code

## Current Project Tasks:
### Short-Term:
- Multitasking:
    - Mutual Exclusion
    - Readers-Writer Lock
- Networking:
    - Firewall/ Packet Filter: NPF or PF & IPFW
    - IPv6
- Filesystem (UFS, FFS & LFS):
- Extend Filesystem Support

### Long-Term:
- Improve Kernel's Dual Control (Proc & User)
- Improve UFS, FFS & LFS
    - Dirhash
    - WAPBL
    - Extended Attributes
- Kernel Modules
- Symmetric Multi-Processing (SMP)
- AMD64/ x86_64 (64-bit)
- Package Management

## Contribution:
- Anyone is welcome to contribute.
- Your code should ideally fit the following guidelines
- Licensing: 3-Clause BSD license Preferable
- These rules in place to make it pleasant for everyone.
	- It is highly recommended that you adhere to the following rules.
	- Failing to do so will prevent your contribution from being accepted.

### Contribution Guidelines:
1. Clean Code.
2. Contain a License Header. Clearly stating year, author and the license.

### Contribution Rules:
1. No profanity
2. No abuse of others
	- Constructive criticism is welcome
