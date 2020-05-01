/*
 * boot.h
 *
 *  Created on: 1 May 2020
 *      Author: marti
 */

#ifndef _SYS_BOOT_H_
#define _SYS_BOOT_H_

/* NetBSD */
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

#define	B_MAGICMASK			((u_long)0xf0000000)
#define	B_DEVMAGIC			((u_long)0xa0000000)

#define MAKEBOOTDEV(type, adaptor, controller, unit, partition) \
	(((type) << B_TYPESHIFT) | ((adaptor) << B_ADAPTORSHIFT) | \
	((controller) << B_CONTROLLERSHIFT) | ((unit) << B_UNITSHIFT) | \
	((partition) << B_PARTITIONSHIFT) | B_DEVMAGIC)

/* FreeBSD */
/*
 * Constants for converting boot-style device number to type,
 * adaptor (uba, mba, etc), unit number and partition number.
 * Type (== major device number) is in the low byte
 * for backward compatibility.  Except for that of the "magic
 * number", each mask applies to the shifted value.
 * Format:
 *	 (4)   (8)   (4)  (8)     (8)
 *	--------------------------------
 *	|MA | SLICE | UN| PART  | TYPE |
 *	--------------------------------
 */
#define B_SLICESHIFT		20
#define B_SLICEMASK			0xff
#define B_SLICE(val)		(((val)>>B_SLICESHIFT) & B_SLICEMASK)
#define B_UNITSHIFT			16
#define B_UNITMASK			0xf
#define	B_UNIT(val)			(((val) >> B_UNITSHIFT) & B_UNITMASK)
#define B_PARTITIONSHIFT	8
#define B_PARTITIONMASK		0xff
#define	B_PARTITION(val)	(((val) >> B_PARTITIONSHIFT) & B_PARTITIONMASK)
#define	B_TYPESHIFT			0
#define	B_TYPEMASK			0xff
#define	B_TYPE(val)			(((val) >> B_TYPESHIFT) & B_TYPEMASK)

#define	B_MAGICMASK	0xf0000000
#define	B_DEVMAGIC	0xa0000000

#define	MAKEBOOTDEV(type, slice, unit, partition) \
	(((type) << B_TYPESHIFT) | ((slice) << B_SLICESHIFT) | \
	((unit) << B_UNITSHIFT) | ((partition) << B_PARTITIONSHIFT) | \
	B_DEVMAGIC)

#define	BASE_SLICE			2
#define	COMPATIBILITY_SLICE	0
#define	MAX_SLICES			32
#define	WHOLE_DISK_SLICE	1

#endif /* SYS_SYS_BOOT_H_ */
