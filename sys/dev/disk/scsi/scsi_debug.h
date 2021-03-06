/*	$NetBSD: scsi_debug.h,v 1.5 1994/12/28 19:43:00 mycroft Exp $	*/

/*
 * Written by Julian Elischer (julian@tfs.com)
 */
#ifndef	_SCSI_SCSI_DEBUG_H
#define _SCSI_SCSI_DEBUG_H 1

/*
 * These are the new debug bits.  (Sat Oct  2 12:46:46 WST 1993)
 * the following DEBUG bits are defined to exist in the flags word of
 * the scsi_link structure.
 */
#define	SDEV_DB1		0x10	/* scsi commands, errors, data	*/ 
#define	SDEV_DB2		0x20	/* routine flow tracking */
#define	SDEV_DB3		0x40	/* internal to routine flows	*/
#define	SDEV_DB4		0x80	/* level 4 debugging for this dev */

/* target and LUN we want to debug */
#define	DEBUGTARGET		-1 		/* -1 = disable */
#define	DEBUGLUN		0
#define	DEBUGLEVEL		(SDEV_DB1|SDEV_DB2)
 
/*
 * This is the usual debug macro for use with the above bits
 */
#ifdef	SCSIDEBUG
#define	SC_DEBUG(sc_link,Level,Printstuff) 	\
	if ((sc_link)->flags & (Level)) {		\
		sc_print_addr(sc_link);				\
 		printf Printstuff;					\
	}
#define	SC_DEBUGN(sc_link,Level,Printstuff) \
	if ((sc_link)->flags & (Level)) {		\
 		printf Printstuff;					\
	}
#else
#define SC_DEBUG(A,B,C)
#define SC_DEBUGN(A,B,C)
#endif

#endif /* _SCSI_SCSI_DEBUG_H */
