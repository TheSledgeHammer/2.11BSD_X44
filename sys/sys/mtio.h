/*	$NetBSD: mtio.h,v 1.21 2003/08/07 16:34:09 agc Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mtio.h	8.1 (Berkeley) 6/2/93
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mtio.h	7.1.2 (2.11BSD) 1998/3/7
 */

#ifndef	_SYS_MTIO_H_
#define	_SYS_MTIO_H_

/*
 * Structures and definitions for mag tape io control commands
 */

/* structure for MTIOCTOP - mag tape op command */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/* operations */
#define MTWEOF		0	/* write an end-of-file record */
#define MTFSF		1	/* forward space file */
#define MTBSF		2	/* backward space file */
#define MTFSR		3	/* forward space record */
#define MTBSR		4	/* backward space record */
#define MTREW		5	/* rewind */
#define MTOFFL		6	/* rewind and put the drive offline */
#define MTNOP		7	/* no operation, sets status only */
#define MTCACHE		8	/* enable controller cache */
#define MTNOCACHE 	9	/* disable controller cache */
#define	MTFLUSH		10	/* flush cache */
#define	MTRETEN		11	/* retension */
#define	MTERASE		12	/* erase entire tape */
#define	MTEOM		13	/* forward to end of media */
#define	MTSETBSIZ	14	/* set block size; 0 for variable */
#define	MTSETDNSTY	15	/* set density code for current mode */
#define	MTCMPRESS	16	/* set/clear device compression */
#define	MTEWARN		17	/* set/clear early warning behaviour */

/* structure for MTIOCGET - mag tape get status command */

struct	mtget	{
	short	mt_type;			/* type of magtape device */
/* the following two registers are grossly device dependent */
	short	mt_dsreg;			/* ``drive status'' register */
	short	mt_erreg;			/* ``error'' register */
/* end device-dependent registers */
	short	mt_resid;			/* residual count */
/* the following two are not yet implemented */
	daddr_t	mt_fileno;			/* file number of current position */
	daddr_t	mt_blkno;			/* block number of current position */
/* end not yet implemented */
	int32_t	mt_blksiz;			/* current block size */
	int32_t	mt_density;			/* current density code */
	int32_t	mt_mblksiz[4];		/* block size for different modes */
	int32_t mt_mdensity[4];		/* density codes for different modes */
};

/*
 * Constants for mt_type byte.  These are the same
 * for other controllers compatible with the types listed.
 */
#define	MT_ISTS		0x01		/* TS-11 */
#define	MT_ISHT		0x02		/* TM03 Massbus: TE16, TU45, TU77 */
#define	MT_ISTM		0x03		/* TM11/TE10 Unibus */
#define	MT_ISMT		0x04		/* TM78/TU78 Massbus */
#define	MT_ISUT		0x05		/* SI TU-45 emulation on Unibus */
#define	MT_ISCPC	0x06		/* SUN */
#define	MT_ISAR		0x07		/* SUN */
#define	MT_ISTMSCP	0x08		/* DEC TMSCP protocol (TU81, TK50) */

/*
 * At present only the TMSCP driver reports this information in the
 * high byte of the 'drive status' word.  Other drives will (hopefully)
 * be updated in the future.
*/
#define	MTF_BOM		0x01		/* At beginning of media */
#define	MTF_EOM		0x02		/* At the end of media */
#define	MTF_OFFLINE	0x04		/* Drive is offline */
#define	MTF_WRTLCK	0x08		/* Drive is write protected */
#define	MTF_WRITTEN	0x10		/* Tape has been written */

/* bits defined for the mt_dsreg field */
#define	MT_DS_RDONLY	0x10		/* tape mounted readonly */
#define	MT_DS_MOUNTED	0x03		/* tape mounted (for control opens) */

/* mag tape io control commands */
#define	MTIOCTOP	_IOW('m', 1, struct mtop)	/* do a mag tape op */
#define	MTIOCGET	_IOR('m', 2, struct mtget)	/* get tape status */
#define MTIOCIEOT	_IO('m', 3)			/* ignore EOT error */
#define MTIOCEEOT	_IO('m', 4)			/* enable EOT error */
/*
 * When more SCSI-3 SSC (streaming device) devices are out there
 * that support the full 32 byte type 2 structure, we'll have to
 * rethink these ioctls to support all the entities they haul into
 * the picture (64 bit blocks, logical file record numbers, etc..).
 */
#define	MTIOCRDSPOS	_IOR('m', 5, uint32_t)	/* get logical blk addr */
#define	MTIOCRDHPOS	_IOR('m', 6, uint32_t)	/* get hardware blk addr */
#define	MTIOCSLOCATE	_IOW('m', 5, uint32_t)	/* seek to logical blk addr */
#define	MTIOCHLOCATE	_IOW('m', 6, uint32_t)	/* seek to hardware blk addr */

#ifndef _KERNEL
#define	DEFTAPE	"/dev/rmt8"
#define	MT_DEF	"/dev/nrmt8"
#endif
#endif /* _SYS_MTIO_H_ */
