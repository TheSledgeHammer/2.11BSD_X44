/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)param.h	1.6 (2.11BSD) 1999/9/5
 */

#define	BSD	211		/* 2.11 * 10, as cpp doesn't do floats */

#ifndef	NULL
#define	NULL	0
#endif

#ifndef LOCORE
#include <sys/types.h>
#endif

/*
 * Machine-independent constants
 */
#include <sys/syslimits.h>

#define	MAXLOGNAME		16			/* max login name length */
#define MAXHOSTNAMELEN	256			/* max hostname size */
#define	NMOUNT			6			/* number of mountable file systems */
#define	MAXUPRC			CHILD_MAX	/* max processes per user */
#define	NOFILE			OPEN_MAX	/* max open files per process */
#define	CANBSIZ			256			/* max size of typewriter line */
#define	NCARGS			ARG_MAX		/* # characters in exec arglist */
#define	NGROUPS			NGROUPS_MAX	/* max number groups */

#define	NOGROUP			65535		/* marker for empty group set member */

/* More types and definitions used throughout the kernel. */
#ifdef KERNEL
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/ucred.h>
#include <sys/rtprio.h>
#endif

/* Signals */
#include <sys/signal.h>

/* Machine type dependent parameters.*/
#include <machine/param.h>
#include <machine/limits.h>

/*
 * Priorities
 */
#define	PSWP	0
#define	PINOD	10
#define	PRIBIO	20
#define	PRIUBA	24
#define	PZERO	25
#define	PPIPE	26
#define	PSOCK	26
#define	PWAIT	30
#define	PLOCK	35
#define	PPAUSE	40
#define	PUSER	50

#define	NZERO	0				/* default "nice" */

#define	PRIMASK	0xff
#define	PCATCH	0x100

#define	NBPW	sizeof(int)		/* number of bytes in an integer */

#define	CMASK	026				/* default mask for file creation */
#define	NODEV	(dev_t)(-1)		/* non-existent device */


/*
 * Clustering of hardware pages on machines with ridiculously small
 * page sizes is done here.  The paging subsystem deals with units of
 * CLSIZE pte's describing NBPG (from machine/machparam.h) pages each.
 */
#define	CLBYTES			(CLSIZE*NBPG)
#define	CLOFSET			(CLBYTES-1)
#define	claligned(x)	((((int)(x))&CLOFSET)==0)
#define	CLOFF			CLOFSET
#define	CLSHIFT			(PGSHIFT + CLSIZELOG2)

/* round a number of clicks up to a whole cluster */
#define	clrnd(i)	(((i) + (CLSIZE-1)) &~ ((long)(CLSIZE-1)))

/* CBLOCK is the size of a clist block, must be power of 2 */
#define	CBLOCK	32
#define	CBSIZE	(CBLOCK - sizeof(struct cblock *))				/* data chars/clist */
#define	CROUND	(CBLOCK - 1)									/* clist rounding */

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of most MAXBSIZE units.
 */
#define	MAXBSIZE	MAXPHYS
#define MAXFRAG 	8

/*
 * MAXPATHLEN defines the longest permissable path length
 * after expanding symbolic links. It is used to allocate
 * a temporary buffer from the buffer pool in which to do the
 * name expansion, hence should be a power of two, and must
 * be less than or equal to MAXBSIZE.
 * MAXSYMLINKS defines the maximum number of symbolic links
 * that may be expanded in a path name. It should be set high
 * enough to allow all legitimate uses, but halt infinite loops
 * reasonably quickly.
 */
#define MAXPATHLEN	PATH_MAX
#define MAXSYMLINKS	8

/*
 * Macros for fast min/max.
 */
#ifndef KERNEL
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif

/*
 * Macros for counting and rounding.
 */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

/*
 * MAXMEM is the maximum core per process is allowed.  First number is Kb.
*/
#define	MAXMEM		(300*16)

/*
 * Scale factor for scaled integers used to count %cpu time and load avgs.
 *
 * The number of CPU `tick's that map to a unique `%age' can be expressed
 * by the formula (1 / (2 ^ (FSHIFT - 11))).  The maximum load average that
 * can be calculated (assuming 32 bits) can be closely approximated using
 * the formula (2 ^ (2 * (16 - FSHIFT))) for (FSHIFT < 15).
 *
 * For the scheduler to maintain a 1:1 mapping of CPU `tick' to `%age',
 * FSHIFT must be at least 11; this gives us a maximum load avg of ~1024.
 */
#ifndef KERNEL
#define	FSHIFT	11		/* bits to right of fixed binary point */
#define FSCALE	(1<<FSHIFT)
#endif

#define powertwo(x) (1 << (x))
