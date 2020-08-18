/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)types.h	1.4.1 (2.11BSD) 2000/2/28
 */

#ifndef _SYS_TYPES_H
#define	_SYS_TYPES_H

/* Machine type dependent parameters. */
#include <machine/types.h>
#include <machine/ansi.h>

#include <sys/ansi.h>

/*
 * Basic system types and major/minor device constructing/busting macros.
 */
#define	major(x)			((int)(((int)(x)>>8)&0377))		/* major part of a device */
#define	minor(x)			((int)((x)&0377)) 				/* minor part of a device */
#define	makedev(x,y)		((dev_t)(((x)<<8)|(y)))			/* make a device number */

typedef	unsigned char		u_char;
typedef	unsigned short		u_short;
typedef	unsigned int		u_int;
typedef	unsigned long		u_long;
typedef	unsigned short		ushort;		/* Sys V compatibility */
typedef	unsigned int		uint;		/* Sys V compatibility */

typedef	unsigned long long 	u_quad_t; 	/* quads */
typedef	long long			quad_t;
typedef	quad_t *			qaddr_t;

typedef u_long				fixpt_t;	/* fixed point number */
typedef	u_short				nlink_t;	/* link count */
typedef	long				segsz_t;	/* segment size */

typedef	long				daddr_t;	/* disk address */
typedef	char *				caddr_t;	/* core address */
typedef	u_long				ino_t;		/* inode number*/
typedef	long				swblk_t;	/* swap offset */
typedef	u_long				size_t;
typedef	int	    			ssize_t;
typedef	long				time_t;		/* time? */
typedef	u_long				dev_t;		/* device number */
typedef	quad_t				off_t;		/* file offset */
typedef	u_long				uid_t;		/* user id */
typedef	u_long				gid_t;		/* group id */
typedef	long	    		pid_t;		/* process id */
typedef	u_short				mode_t;		/* permissions */
typedef	register_t			register_t;
typedef u_long 				sigset_t;
typedef	unsigned long		cpuid_t;

 typedef struct	_quad {
	long val[2];
} quad;

#define	NBBY	8			/* number of bits in a byte */

#ifndef howmany
#define	howmany(x, y)		(((x)+((y)-1))/(y))
#endif

#include <sys/select.h>

typedef char				bool_t;		/* boolean */
typedef long				memaddr;	/* core or swap address */

#ifdef	_BSD_CLOCK_T_
typedef	_BSD_CLOCK_T_	clock_t;
#undef	_BSD_CLOCK_T_
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_SSIZE_T_
typedef	_BSD_SSIZE_T_	ssize_t;
#undef	_BSD_SSIZE_T_
#endif

#ifdef	_BSD_TIME_T_
typedef	_BSD_TIME_T_	time_t;
#undef	_BSD_TIME_T_
#endif

#include <sys/stdint.h>

typedef	uint8_t		u_int8_t;
typedef	uint16_t	u_int16_t;
typedef	uint32_t	u_int32_t;
typedef	uint64_t	u_int64_t;

#include <machine/endian.h>

#if defined(_KERNEL) || defined(_STANDALONE)
#define SET(t, f)	((t) |= (f))
#define	ISSET(t, f)	((t) & (f))
#define	CLR(t, f)	((t) &= ~(f))
#endif

#if defined(_KERNEL) || defined(_STANDALONE)

#include <sys/stdbool.h>

/*
 * Deprecated Mach-style boolean_t type.  Should not be used by new code.
 */
typedef int	boolean_t;
#ifndef TRUE
#define	TRUE	1
#endif
#ifndef FALSE
#define	FALSE	0
#endif
#endif /* _KERNEL || _STANDALONE */
#endif
