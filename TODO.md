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
  - Fix to be relavent to 2.11BSD

# usr/ (User & OS Libraries)

## lib

- libc:
	- Update Makefiles with manpage's MLINKS

## libexec

## usr.lib
- libcurses: to add
- libterminfo: 2.11BSD codebase. Very likely needs updating
- libutil: Missing all newer files beyond 4.4BSD-Lite2

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
