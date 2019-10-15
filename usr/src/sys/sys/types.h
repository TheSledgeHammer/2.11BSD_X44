/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)types.h	1.4.1 (2.11BSD) 2000/2/28
 */

#ifndef _TYPES_
#define	_TYPES_

#include <sys/cdefs.h>


/* Machine type dependent parameters. */
#include <machine/endian.h>

/*
 * Basic system types and major/minor device constructing/busting macros.
 */
#define	major(x)		((int)(((int)(x)>>8)&0377))		/* major part of a device */
#define	minor(x)		((int)((x)&0377)) 				/* minor part of a device */
#define	makedev(x,y)	((dev_t)(((x)<<8) | (y)))		/* make a device number */

//typedef unsigned char	u_char;
//typedef unsigned short	u_short;
//typedef unsigned int	u_int;
//typedef unsigned long	u_long;		/* see this! unsigned longs at last! */
//typedef unsigned short	ushort;		/* sys III / Sys V Compat */


/* Obtained from FreeBSD 2.0 */
#ifndef _POSIX_SOURCE
typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		/* Sys V compatibility */
typedef	unsigned int	uint;		/* Sys V compatibility */
#endif

typedef	unsigned long long 	u_quad_t; /* quads */
typedef	long long			quad_t;
typedef	quad_t *			qaddr_t;

typedef unsigned long	fixpt_t;	/* fixed point number */
typedef	unsigned short	nlink_t;	/* link count */
typedef	long			segsz_t;	/* segment size */
/* End of FreeBSD 2.0 types.h Content */

typedef	long			daddr_t;	/* disk address */
typedef	char *			caddr_t;	/* core address */
typedef	unsigned long	ino_t;		/* inode number*/
typedef	long			swblk_t;	/* swap offset */
typedef	u_int			size_t;		/* pdp-11 size? */
typedef	int	    		ssize_t;	/* pdp-11 size? */
typedef	long			time_t;		/* time? */
typedef	unsigned long	dev_t;		/* device number */
typedef	quad_t			off_t;		/* file offset */
typedef	unsigned long	uid_t;		/* user id */
typedef	unsigned long	gid_t;		/* group id */
typedef	long	    	pid_t;		/* process id */
typedef	unsigned short	mode_t;		/* permissions */

/* Does this relate to quads above? */
 typedef	struct	_quad {
	long val[2];
} quad;

#define	NBBY	8		/* number of bits in a byte */

#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

#include <sys/select.h>

typedef char	bool_t;		/* boolean */
typedef size_t	memaddr;	/* core or swap address */
