/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)types.h	1.4.1 (2.11BSD) 2000/2/28
 */

#ifndef _SYS_TYPES_H
#define	_SYS_TYPES_H

#include <sys/ansi.h>
#include <sys/select.h>
#include <sys/stdint.h>

/* Machine type dependent parameters. */
#include <machine/ansi.h>
#include <machine/endian_machdep.h>
#include <machine/types.h>

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
typedef unsigned long		vaddr_t;	/* virtual address */
typedef	u_long				ino_t;		/* inode number*/
typedef	long				swblk_t;	/* swap offset */
typedef	u_long				size_t;
typedef	int	    			ssize_t;
typedef	long				time_t;		/* time? */
typedef	u_long				dev_t;		/* device number */
typedef	quad_t				off_t;		/* file offset */
typedef	u_long				uid_t;		/* user id */
typedef	u_long				gid_t;		/* group id */
typedef	u_char	    		pid_t;		/* process id */
typedef	u_short				mode_t;		/* permissions */
typedef	unsigned long		cpuid_t;
typedef char				bool_t;		/* boolean */
typedef long				memaddr;	/* core or swap address */

 typedef struct	_quad {
	long 					val[2];
} quad;

/*
 * Basic system types and major/minor device constructing/busting macros.
 */
#define	major(x)			((int)(((int)(x)>>8)&0377))		/* major part of a device */
#define	minor(x)			((int)((x)&0377)) 				/* minor part of a device */
#define	makedev(x,y)		((dev_t)(((x)<<8)|(y)))			/* make a device number */

#define	NBBY				8								/* number of bits in a byte */

#ifndef howmany
#define	howmany(x, y)		(((x)+((y)-1))/(y))
#endif

#ifndef	_BSD_INT8_T_
typedef	__int8_t			int8_t;
#define	_BSD_INT8_T_
#endif

#ifndef	_BSD_UINT8_T_
typedef	__uint8_t			uint8_t;
#define	_BSD_UINT8_T_
#endif

#ifndef	_BSD_INT16_T_
typedef	__int16_t			int16_t;
#define	_BSD_INT16_T_
#endif

#ifndef	_BSD_UINT16_T_
typedef	__uint16_t			uint16_t;
#define	_BSD_UINT16_T_
#endif

#ifndef	_BSD_INT32_T_
typedef	__int32_t			int32_t;
#define	_BSD_INT32_T_
#endif

#ifndef	_BSD_UINT32_T_
typedef	__uint32_t			uint32_t;
#define	_BSD_UINT32_T_
#endif

#ifndef	_BSD_INT64_T_
typedef	__int64_t			int64_t;
#define	_BSD_INT64_T_
#endif

#ifndef	_BSD_UINT64_T_
typedef	__uint64_t			uint64_t;
#define	_BSD_UINT64_T_
#endif

#ifdef	_BSD_CLOCK_T_
typedef	_BSD_CLOCK_T_		clock_t;
#undef	_BSD_CLOCK_T_
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_		size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_SSIZE_T_
typedef	_BSD_SSIZE_T_		ssize_t;
#undef	_BSD_SSIZE_T_
#endif

#ifdef	_BSD_TIME_T_
typedef	_BSD_TIME_T_		time_t;
#undef	_BSD_TIME_T_
#endif

#ifdef	_BSD_CLOCKID_T_
typedef	_BSD_CLOCKID_T_		clockid_t;
#undef	_BSD_CLOCKID_T_
#endif

#ifdef	_BSD_TIMER_T_
typedef	_BSD_TIMER_T_		timer_t;
#undef	_BSD_TIMER_T_
#endif

#if defined(_KERNEL) || defined(_STANDALONE)
#define SET(t, f)			((t) |= (f))
#define	ISSET(t, f)			((t) & (f))
#define	CLR(t, f)			((t) &= ~(f))
#endif

#if defined(__STDC__) && defined(_KERNEL)
/*
 * Forward structure declarations for function prototypes.  We include the
 * common structures that cross subsystem boundaries here; others are mostly
 * used in the same place that the structure is defined.
 */
struct	proc;
struct	pgrp;
struct	ucred;
struct	rusage;
struct	k_rusage;
struct	file;
struct	buf;
struct	tty;
struct	uio;
struct	user;
#endif

#if defined(_KERNEL) || defined(_STANDALONE)

#include <sys/stdbool.h>
/*
 * Deprecated Mach-style boolean_t type.  Should not be used by new code.
 */
typedef int		boolean_t;
#ifndef TRUE
#define	TRUE	1
#endif
#ifndef FALSE
#define	FALSE	0
#endif
#endif /* _KERNEL || _STANDALONE */
#endif
