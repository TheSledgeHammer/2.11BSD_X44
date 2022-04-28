# 2.11BSD_X44 (TBA)

2.11BSD is unique in BSD's history, still receiving patches to it's codebase up until the present. As well as being the only BSD to receive patches that were backported from numerous versions of 4BSD.

2.11BSD_X44 is 2.11BSD that continues with that tradition. Replacing the 4BSD (4.1BSD to 4.3BSD) styled vmspace and inodes with 4.4BSD's vmspace and vnodes. While retaining 2.11BSD's kernel and user space. 2.11BSD_X44 adopts the 4.4BSD & later BSD's (i.e. FreeBSD, NetBSD, OpenBSD & DragonflyBSD) approach of having a clearly defined architecture dependent code and architecture independent code, allowing for easier portability.

## Source Roadmap:
---------------
| Directory | Description |
| --------- | ----------- |
| bin | System/user commands. |
| contrib | Packages contributed by 3rd parties. |
| etc | Template files for /etc. |
| include | System include files. |
| lib | System libraries. |
| libexec | System daemons. |
| sbin | System commands. |
| share | Shared resources. |
| sys | Kernel sources. |
| sys/arch/`arch`/conf | Kernel configuration files. GENERIC.'arch' is the configuration used in release builds. |
| stand | Boot loader sources. |
| tools | Utilities for toolchain & cross-compiling. |
| usr.bin | User commands. |
| usr.lib | System User libraries for /lib. |
| usr.sbin | System administration commands. |

## Architecture Support:

- i386/x86 (In Development)
- AMD64 (Planned)
- Arm 	(32-Bit/64-Bit) (Planned)
- Riscv (32-Bit/64-Bit) (Planned)

## Project Goals:

- Maintain a coding style in keeping with 2.11BSD's
- Innovate new ways to improve the OS while following the above point.
- Small Footprint: Ideally less than 200 system calls.
- Clean code

## Project Aims:

- Improve 2.11BSD's Dual Control (Proc & User)
- Instruction & Data (I&D) Seperation (See devel/vm folder)
- Pseudo Segmentation (See devel/vm folder)
- Threading (Hybrid N/M):  
  - Kernel Threads:
    - mcontext & ucontext
    - Bug Testing
  - User Threads/Fibres:
    - PThreads Support
    - IPC Improvements
- Scheduler:
  - Implement Preemptive capabilities
  - Bug Testing
- Networking:
  - Firewall/ Packet Filter: NPF, PF & IPFilter
  - IPv6
- Filesystem Updates:
  - UFS/FFS/LFS:
    - LFS1/LFS2: (Work in Progress: See TODO.md for more)
      - LFS2: 64-bit support via UFS/FFS2
      - LFS1: 32-bit support via UFS/FFS1
    - UFS1/UFS2:
      - Journaling
      - Dirhash
- Filesystems to Add:
  - EXT2/3/4
  - NTFS
  - HPFS
  - HFS
  - NFS
  - SMB/CIFS
  - UDF
  - PUFFS or FUSE

## Development:

### Started:
- Please read the TODO for an in-depth list.

OR

- Read the README.md in "devel" for in-development concepts planned

### To Start:
- DEC VT220 style: amber coloured terminal font
- Package Manager: Port FreeBSD's
- BSD Installer: Port DragonflyBSD's

## Porting:

2.11BSD_X44 is open to being ported to different architectures.
Though due to limited access to hardware, testing of 2.11BSD_X44 on that architecture will be dependent on the individual/group (especially more exotic hardware).

## Building:
You can cross-build 211BSD_X44 from most UNIX-like operating systems. To build for i386, in the src directory:

./build.sh -U -u -j4 -m i386 -O ~/obj release

NOTICE: The above build script will not produce a successful build.
It is only recommended at this stage for testing the cross-compiler or adding new tools.

To build just the tools for i386, in the src directory:

./build.sh -U -u -j4 -m i386 -O ~/obj tools

Compilers:

Clang: Yes (but fails during llvm support) library very likely needs updating along with the relevent toolchain components.
PCC: No.

GCC: Yes: i386(tools = yes; kernel = fix compilation errors)

Please read the following NetBSD guide for more information:

<https://www.netbsd.org/docs/guide/en/chap-build.html>

## Contribution:

- Anyone is welcome to contribute.
- Your code should ideally fit the following guidelines & rules below
- Licensing: 
  - The 3-Clause BSD license preferred.
  - Externally Maintained code for user libraries should be placed in one of the following:
  	1) Any licensed code: 		contrib/"project name"
  	2) OS Compatability code:  	contrib/oscompat/"os name"
- These rules in place to make it pleasant & easier for everyone.
  - It is highly recommended that you adhere to the following Contribution Guidelines & Rules.
  - Failing to follow the Contribution Guidelines & Rules will likely prevent your code from being accepted.

### Contribution Guidelines

1. Clean Code.
2. Must Contain a License Header. Clearly stating year, author and the license.

### Contribution Rules

1. Use of profanity.
   - Will not accept code that contains profanity.
2. No abuse of others
   - Constructive criticism is welcome
