# 2.11BSD_X44 (TBA)
2.11BSD is unique in BSD's history, still receiving patches to it's codebase up until the last 10 years. As well as being the only BSD to receive patches that were backported from numerous versions of 4BSD.

2.11BSD_X44 is 2.11BSD that continues with that tradition. Replacing the 4.3BSD styled VM space and inodes with 4.4BSD-Lite2 VM space and vnodes. While retaining 2.11BSD's kernel and user space. 2.11BSD_X44 adopts the 4.4BSD & later BSD's (i.e. FreeBSD, NetBSD, OpenBSD & DragonflyBSD) approach of having a clearly defined architecture dependent code and architecture independent code, allowing for easier portability.

## Project Goals:
- Maintain Traditional 2.11BSD Style while keeping current.
  - Current focus is on re-using & extending as much of the 2.11BSD codebase where applicable before importing new or existing code
- Small Footprint: Ideally less than 200 system calls.
- Clean code

## Architecture Support:
- i386 (In Development)
- amd64 (Planned)

## Current Project Tasks:
### Short-Term:
- Multitasking:
    - Threading
    - Mutual Exclusion
    - Readers-Writer Lock
- Networking:
    - Firewall/ Packet Filter: NPF or PF & IPFW
    - IPv6
- Filesystem Updates for UFS, FFS & LFS

### Long-Term:
- Improve 2.11BSD's Dual Control (Proc & User)
- Improve UFS, FFS & LFS
    - Dirhash
    - WAPBL
    - Extended Attributes
- Filesystem Support For: (Needs improved network features first)
  - NFS
  - SMB
- Kernel Modules
- Symmetric Multi-Processing (SMP)

## Development:
- Please read the TODO for a list of what needs doing.

## Porting:
2.11BSD_X44 is entirely open to being ported to different architectures.
Though due to limited access to hardware, testing of 2.11BSD_X44 on that architecture will be dependent on the individual/group (especially more exotic hardware).

## Contribution:
- Anyone is welcome to contribute.
- Your code should ideally fit the following guidelines & rules below
- Licensing: 3-Clause BSD license preferable
  - Non BSD licensed code should be placed in the (folder: external/"license"/"project name")
    - Different versions of the same License (e.g. GPLv1, GPLv2, etc.) can be placed under one folder
- These rules in place to make it pleasant & easier for everyone.
	- It is highly recommended that you adhere to the following Contribution Guidelines & Rules.
	- Failing to follow the Contribution Guidelines & Rules will prevent your code from being accepted.

### Contribution Guidelines:
1. Clean Code.
2. Contain a License Header. Clearly stating year, author and the license.

### Contribution Rules:
1. No excessive use of profanity
2. No abuse of others
	- Constructive criticism is welcome
