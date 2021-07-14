# 2.11BSD_X44 (TBA)

2.11BSD is unique in BSD's history, still receiving patches to it's codebase up until the last 10 years. As well as being the only BSD to receive patches that were backported from numerous versions of 4BSD.

2.11BSD_X44 is 2.11BSD that continues with that tradition. Replacing the 4.3BSD styled vmspace and inodes with 4.4BSD's vmspace and vnodes. While retaining 2.11BSD's kernel and user space. 2.11BSD_X44 adopts the 4.4BSD & later BSD's (i.e. FreeBSD, NetBSD, OpenBSD & DragonflyBSD) approach of having a clearly defined architecture dependent code and architecture independent code, allowing for easier portability.

## Project Goals

- Maintain a coding style in keeping with 2.11BSD's
- Innovate new ways to improve the OS while following the above point.
- Small Footprint: Ideally less than 200 system calls.
- Clean code

## Architecture Support

- i386/x86 (In Development)
  - 95% Complete: without SMP, Threads & limited PAE
  - 60-80% Complete: with SMP, PAE & limited Threads
- AMD64 (Planned)
- Arm (32-Bit/64-Bit) (Planned)

## Current Project Aims

- Improve 2.11BSD's Dual Control (Proc & User)
- Threading (Hybrid N/M):  
  - Kernel Threads: (85% Complete)
    - Machine-Dependent Code
    - Bug Testing
  - User Threads/Fibres: Work in Progress (70% Complete)
    - PThreads Support
    - IPC Improvements
- Scheduler: (95% Complete)
  - Bug Testing
- Networking:
  - Firewall/ Packet Filter: NPF or PF & IPFW
  - IPv6
- Filesystem Updates:
  - UFS/FFS/LFS:
    - LFS1/LFS2:
      - LFS2: 64-bit support via UFS/FFS2
      - LFS1: 32-bit support via UFS/FFS1
    - UFS1/UFS2:
      - Journaling:
      - Dirhash: Work in Progress
- New Filesystems (In Kernel):
  - Union (BSD's)
  - EXT2/3/4
  - NTFS
  - HPFS
  - HFS
  - UDF
  - NFS
  - SMB/CIFS
  - PUFFS or FUSE

## Development

- Please read the TODO for an in-depth list.
Or
- Read the README.md in "devel" for in-development concepts planned

- Top Priority:
  - Compiling for i386/x86 platform

## Porting

2.11BSD_X44 is entirely open to being ported to different architectures.
Though due to limited access to hardware, testing of 2.11BSD_X44 on that architecture will be dependent on the individual/group (especially more exotic hardware).

## Building: (Work in Progress)
You can cross-build 211BSD_X44 from most UNIX-like operating systems. To build for i386, in the src directory:

./build.sh -U -u -j4 -m i386 -O ~/obj release

NOTICE: The above build script will not produce a successful build.
It is only recommended at this stage for testing the cross-compiler or adding new tools.
To build just the tools for i386, in the src directory:

./build.sh -U -u -j4 -m i386 tools

Please read the following NetBSD guide for more information: 

<https://www.netbsd.org/docs/guide/en/chap-build.html>

## Contribution

- Anyone is welcome to contribute.
- Your code should ideally fit the following guidelines & rules below
- Licensing: 3-Clause BSD license preferable
  - Non BSD licensed code should be placed in the (folder: contrib/"license"/"project name")
  - Different versions of the same License (e.g. GPLv1, GPLv2, etc.) can be placed under one folder
- These rules in place to make it pleasant & easier for everyone.
  - It is highly recommended that you adhere to the following Contribution Guidelines & Rules.
  - Failing to follow the Contribution Guidelines & Rules will prevent your code from being accepted.

### Contribution Guidelines

1. Clean Code.
2. Must Contain a License Header. Clearly stating year, author and the license.

### Contribution Rules

1. Use of profanity.
   - Will not accept code that contains profanity.
2. No abuse of others

- Constructive criticism is welcome
