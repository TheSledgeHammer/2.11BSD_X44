/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)types.h	1.4.1 (2.11BSD) 2000/2/28
 */

#ifndef SYS_TYPES_H
#define	SYS_TYPES_H

#include <sys/cdefs.h>
#include <sys/ansi.h>

/* Machine type dependent parameters. */
#include <machine/endian.h>
#include <machine/ansi.h>
#include <machine/types.h>

/*
 * Basic system types and major/minor device constructing/busting macros.
 */
#define	major(x)		((int)(((int)(x)>>8)&0377))		/* major part of a device */
#define	minor(x)		((int)((x)&0377)) 				/* minor part of a device */
#define	makedev(x,y)	((dev_t)(((x)<<8) | (y)))		/* make a device number */

typedef	unsigned char		u_char;
typedef	unsigned short		u_short;
typedef	unsigned int		u_int;
typedef	unsigned long		u_long;
typedef	unsigned short		ushort;		/* Sys V compatibility */
typedef	unsigned int		uint;		/* Sys V compatibility */

typedef	unsigned long long 	u_quad_t; 	/* quads */
typedef	long long			quad_t;
typedef	quad_t *			qaddr_t;

typedef unsigned long		fixpt_t;	/* fixed point number */
typedef	unsigned short		nlink_t;	/* link count */
typedef	long				segsz_t;	/* segment size */

typedef	long				daddr_t;	/* disk address */
typedef	char *				caddr_t;	/* core address */
typedef	unsigned long		ino_t;		/* inode number*/
typedef	long				swblk_t;	/* swap offset */
typedef	unsigned long		size_t;		/* pdp-11 size? */
typedef	int	    			ssize_t;	/* pdp-11 size? */
typedef	long				time_t;		/* time? */
typedef	unsigned long		dev_t;		/* device number */
typedef	quad_t				off_t;		/* file offset */
typedef	unsigned long		uid_t;		/* user id */
typedef	unsigned long		gid_t;		/* group id */
typedef	long	    		pid_t;		/* process id */
typedef	unsigned short		mode_t;		/* permissions */
typedef	register_t			register_t;

 typedef struct	_quad {
	long val[2];
} quad;

#define	NBBY	8			/* number of bits in a byte */

#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

typedef char	bool_t;		/* boolean */
typedef size_t	memaddr;	/* core or swap address */

typedef	unsigned int intrmask_t;	/* Interrupt mask (spl, xxx_imask...) */
intrmask_t	splsoftcam __P((void));
intrmask_t	splsoftcambio __P((void));
intrmask_t	splsoftcamnet __P((void));
intrmask_t	splsoftclock __P((void));
intrmask_t	splsofttty __P((void));
intrmask_t	splsoftvm __P((void));
intrmask_t	splstatclock __P((void));
intrmask_t	spltty __P((void));
intrmask_t	splvm __P((void));
void		splx __P((intrmask_t ipl));
void		splz __P((void));

#ifndef	int8_t
typedef	int8_t		int8_t;
#define	int8_t		int8_t
#endif

#ifndef	uint8_t
typedef	u_int8_t	uint8_t;
#define	uint8_t		u_int8_t
#endif

#ifndef	int16_t
typedef	int16_t		int16_t;
#define	int16_t		int16_t
#endif

#ifndef	uint16_t
typedef	u_int16_t	uint16_t;
#define	uint16_t	u_int16_t
#endif

#ifndef	int32_t
typedef	int32_t		int32_t;
#define	int32_t		int32_t
#endif

#ifndef	uint32_t
typedef	u_int32_t	uint32_t;
#define	uint32_t	u_int32_t
#endif

#ifndef	int64_t
typedef	int64_t		int64_t;
#define	int64_t		int64_t
#endif

#ifndef	uint64_t
typedef	u_int64_t	uint64_t;
#define	uint64_t	u_int64_t
#endif

typedef	uint8_t		u_int8_t;
typedef	uint16_t	u_int16_t;
typedef	uint32_t	u_int32_t;
typedef	uint64_t	u_int64_t;
#endif
