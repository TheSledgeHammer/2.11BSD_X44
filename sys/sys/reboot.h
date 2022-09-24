/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)reboot.h	1.2 (2.11BSD GTE) 1996/5/9
 */
#ifndef _SYS_REBOOT_H_
#define _SYS_REBOOT_H_

/*
 * Arguments to reboot system call.
 * These are passed to boot program in r4 or r11,
 * and on to init.
 */
#define	RB_AUTOBOOT		0			/* flags for system auto-booting itself */
#define	RB_BOOT			1			/* reboot due to boot() */
#define	RB_PANIC		2			/* reboot due to panic */

#define	RB_ASKNAME		0x001		/* ask for file name to reboot from */
#define	RB_SINGLE		0x002		/* reboot to single user only */
#define	RB_NOSYNC		0x004		/* dont sync before reboot */
#define	RB_HALT			0x008		/* don't reboot, just halt */
#define	RB_INITNAME		0x010		/* name given for /etc/init */
#define	RB_DFLTROOT		0x020		/* use compiled-in rootdev */
#define	RB_DUMP			0x040		/* take a dump before rebooting */
#define	RB_NOFSCK		0x080		/* don't perform fsck's on reboot */
#define	RB_POWRFAIL		0x100		/* reboot caused by power failure */
#define	RB_RDONLY		0x200		/* mount root fs read-only */
#define	RB_AUTODEBUG	0x400		/* init runs autoconfig with "-d" (debug) */
#define RB_VERBOSE		0x800		/* print all potentially useful info */
#define	RB_SERIAL		0x1000		/* use serial port as console */
#define RB_CDROM		0x2000		/* use cdrom as root */
#define RB_KDB			0x4000		/* give control to kernel debugger */
#define RB_GDB			0x8000		/* use GDB remote debugger instead of DDB */
#define	RB_MUTE			0x10000		/* start up with the console muted */
#define RB_PAUSE		0x100000	/* pause after each output line during probe */
#define RB_PROBE		0x10000000	/* Probe multiple consoles */
#define	RB_MULTIPLE		0x20000000	/* use multiple consoles */

#define	RB_BOOTINFO		0x80000000	/* have `struct bootinfo *' arg */

#ifndef _LOCORE
/*
 * Constants for converting boot-style device number to type,
 * adaptor (uba, mba, etc), unit number and partition number.
 * Type (== major device number) is in the low byte
 * for backward compatibility.  Except for that of the "magic
 * number", each mask applies to the shifted value.
 * Format:
 *
 * 	Slices:
 *	 (4)   (8)   (4)  (8)     (8)
 *	--------------------------------
 *	|MA | SLICE | UN| PART  | TYPE |
 *	--------------------------------
 *
 *	Traditional:
 *	 (4) (4) (4) (4)  (8)     (8)
 *	--------------------------------
 *	|MA | AD| CT| UN| PART  | TYPE |
 *	--------------------------------
 */

/* Traditional: Adaptor & Controller */
#define	B_ADAPTORSHIFT		24
#define	B_ADAPTORMASK		0x0f
#define	B_ADAPTOR(val)		(((val) >> B_ADAPTORSHIFT) & B_ADAPTORMASK)
#define B_CONTROLLERSHIFT	20
#define B_CONTROLLERMASK	0xf
#define	B_CONTROLLER(val)	(((val)>>B_CONTROLLERSHIFT) & B_CONTROLLERMASK)

/* Slices: */
#define B_SLICESHIFT		20
#define B_SLICEMASK			0xff
#define B_SLICE(val)		(((val)>>B_SLICESHIFT) & B_SLICEMASK)

/* Common: */
#define B_UNITSHIFT			16
#define B_UNITMASK			0xf
#define	B_UNIT(val)			(((val) >> B_UNITSHIFT) & B_UNITMASK)
#define B_PARTITIONSHIFT	8
#define B_PARTITIONMASK		0xff
#define	B_PARTITION(val)	(((val) >> B_PARTITIONSHIFT) & B_PARTITIONMASK)
#define	B_TYPESHIFT			0
#define	B_TYPEMASK			0xff
#define	B_TYPE(val)			(((val) >> B_TYPESHIFT) & B_TYPEMASK)

#define	B_MAGICMASK			((u_long)0xf0000000)
#define	B_DEVMAGIC			((u_long)0xa0000000)

/* Traditional boot style (without slices) */
#define MAKEBOOTDEV1(type, adaptor, controller, unit, partition) 	\
	(((type) << B_TYPESHIFT) | ((adaptor) << B_ADAPTORSHIFT) | 		\
	((controller) << B_CONTROLLERSHIFT) | ((unit) << B_UNITSHIFT) | \
	((partition) << B_PARTITIONSHIFT) | B_DEVMAGIC)

/* Slices boot style (with slices) */
#define	MAKEBOOTDEV2(type, slice, unit, partition) 					\
	(((type) << B_TYPESHIFT) | ((slice) << B_SLICESHIFT) | 			\
	((unit) << B_UNITSHIFT) | ((partition) << B_PARTITIONSHIFT) | 	\
	B_DEVMAGIC)

/* Slice information */
#define	BASE_SLICE			2
#define	COMPATIBILITY_SLICE	0
#define	MAX_SLICES			32
#define	WHOLE_DISK_SLICE	1

#endif /* !_LOCORE */
#endif
