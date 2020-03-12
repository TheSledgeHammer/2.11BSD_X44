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
#define	RB_AUTOBOOT		0		/* flags for system auto-booting itself */

#define	RB_ASKNAME		0x001	/* ask for file name to reboot from */
#define	RB_SINGLE		0x002	/* reboot to single user only */
#define	RB_NOSYNC		0x004	/* dont sync before reboot */
#define	RB_HALT			0x008	/* don't reboot, just halt */
#define	RB_INITNAME		0x010	/* name given for /etc/init */
#define	RB_DFLTROOT		0x020	/* use compiled-in rootdev */
#define	RB_DUMP			0x040	/* take a dump before rebooting */
#define	RB_NOFSCK		0x080	/* don't perform fsck's on reboot */
#define	RB_POWRFAIL		0x100	/* reboot caused by power failure */
#define	RB_RDONLY		0x200	/* mount root fs read-only */
#define	RB_AUTODEBUG	0x400	/* init runs autoconfig with "-d" (debug) */

#define	RB_PANIC		0		/* reboot due to panic */
#define	RB_BOOT			1		/* reboot due to boot() */

#define	RB_BOOTINFO		0x80000000	/* have `struct bootinfo *' arg */

/*
 * Constants for converting boot-style device number to type,
 * adaptor (uba, mba, etc), unit number and partition number.
 * Type (== major device number) is in the low byte
 * for backward compatibility.  Except for that of the "magic
 * number", each mask applies to the shifted value.
 * Format:
 *	 (4) (4) (4) (4)  (8)     (8)
 *	--------------------------------
 *	|MA | AD| CT| UN| PART  | TYPE |
 *	--------------------------------
 */
#define	B_ADAPTORSHIFT		24
#define	B_ADAPTORMASK		0x0f
#define	B_ADAPTOR(val)		(((val) >> B_ADAPTORSHIFT) & B_ADAPTORMASK)
#define B_CONTROLLERSHIFT	20
#define B_CONTROLLERMASK	0xf
#define	B_CONTROLLER(val)	(((val)>>B_CONTROLLERSHIFT) & B_CONTROLLERMASK)
#define B_UNITSHIFT			16
#define B_UNITMASK			0xf
#define	B_UNIT(val)			(((val) >> B_UNITSHIFT) & B_UNITMASK)
#define B_PARTITIONSHIFT	8
#define B_PARTITIONMASK		0xff
#define	B_PARTITION(val)	(((val) >> B_PARTITIONSHIFT) & B_PARTITIONMASK)
#define	B_TYPESHIFT			0
#define	B_TYPEMASK			0xff
#define	B_TYPE(val)			(((val) >> B_TYPESHIFT) & B_TYPEMASK)

#define	B_MAGICMASK	((u_long)0xf0000000)
#define	B_DEVMAGIC	((u_long)0xa0000000)

#define MAKEBOOTDEV(type, adaptor, controller, unit, partition) \
	(((type) << B_TYPESHIFT) | ((adaptor) << B_ADAPTORSHIFT) | \
	((controller) << B_CONTROLLERSHIFT) | ((unit) << B_UNITSHIFT) | \
	((partition) << B_PARTITIONSHIFT) | B_DEVMAGIC)

#endif
