A General todo list. Don't hesitate to add to this list. :)

# TODO

## General

- Compiler
- Makefiles
- Bug Fixes &/or missing critical content
- Update sys/dev & sys/stand/boot Makefiles

## Tools (NetBSD)
- compat (nbcompat): 
  - can be simplified using the compat_xx.h as a rough guide for what it requires from lib

- Long-Term Goal:
  - Support: Clang, GCC & PCC
  - Have all compiler components under a single directory.
   	- E.g. Compiler Folder:
      - GCC: binutils, flex(optional), gcc, gdb, gmake, groff, m4, mpc, mpfr
      - Clang: clang, llvm, compiler-rt

# usr/ (User & OS Libraries)

## lib

- libkvm
  
## libexec

## sbin

- fsck: replace references to ufs_daddr_t
  
# usr/sys/ (Kernel)

## conf

## kern

- event/kqfilter: implemented but unused.

## arch

- i386/x86: (Merged under i386)
  - smp/multi-cpu:
    - machine-independent code: 90% complete
      - smp-related methods for cpu
    - machine-dependent code: 75% complete
      - boot: considering FreeBSD's mpboot.s
      - smp: alloction to assign interrupts to CPUs
      - tsc: missing struct timecounter

## devel: (planned)

- Code planned for future integration
- update copyright headers
- See devel folder: README.md

## dev

- Essential Driver Support:
  - usb:         Work in progress
    - add: vhci, xhci
  - wscons/pccons:      Work in progress
    - double check wscons for errors/mistakes
- cfops update:
  - double check com.c needs cfdriver declaration
  - to use detach & activate routines

## fs

## libkern

## libsa

## net / netimp / netinet / netns

Of Interest Todo:

- 2.11BSD's networking stack
  - To Support:
    - ipv6
    - firewall/packet filter
    - plus much more

## stand

- boot:
  - fix makefiles to compile correct selected architechture from build environment.
    i.e. i386 machine should only compile what is needed for the i386 bootloader
  - configurable
    - boot format: FreeBSD Slices & NetBSD Adaptor & Controller
      - failsafe: Cannot boot if both are true or both are false
  - efi
  - commands: needs work
  - install: not present

## ufs

## vm

## Other

- Memory Segmentation (Software):
  - Seperate Process Segments: text, data, stack
  - Seperate Instruction & Data Spaces
