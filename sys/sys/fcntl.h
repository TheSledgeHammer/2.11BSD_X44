/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)fcntl.h	5.2 (Berkeley) 1/8/86
 */
#ifndef _SYS_FCNTL_H_
#define	_SYS_FCNTL_H_
/*
 * Flag values accessible to open(2) and fcntl(2)-- copied from
 * <sys/file.h>.  (The first three can only be set by open.)
 */

#ifndef KERNEL
#include <sys/types.h>
#endif


#define	O_RDONLY	000			/* open for reading */
#define	O_WRONLY	001			/* open for writing */
#define	O_RDWR		002			/* open for read & write */
#define	O_NDELAY	FNDELAY		/* non-blocking open */
								/* really non-blocking I/O for fcntl */
#define	O_APPEND	FAPPEND		/* append on each write */
#define	O_CREAT		FCREAT		/* open with file create */
#define	O_TRUNC		FTRUNC		/* open with truncation */
#define	O_EXCL		FEXCL		/* error on create if file exists */

#ifndef	F_DUPFD
/* fcntl(2) requests */
#define	F_DUPFD	0	/* Duplicate fildes */
#define	F_GETFD	1	/* Get fildes flags */
#define	F_SETFD	2	/* Set fildes flags */
#define	F_GETFL	3	/* Get file flags */
#define	F_SETFL	4	/* Set file flags */
#define	F_GETOWN 5	/* Get owner */
#define F_SETOWN 6	/* Set owner */

/* flags for F_GETFL, F_SETFL-- copied from <sys/file.h> */
#define	FNDELAY		00004		/* non-blocking reads */
#define	FAPPEND		00010		/* append on each write */
#define	FASYNC		00100		/* signal pgrp when data ready */
#define	FCREAT		01000		/* create if nonexistant */
#define	FTRUNC		02000		/* truncate to zero length */
#define	FEXCL		04000		/* error if already created */
#endif

/*
 * Kernel encoding of open mode; separate read and write bits that are
 * independently testable: 1 greater than the above.
 *
 * XXX
 * FREAD and FWRITE are excluded from the #ifdef KERNEL so that TIOCFLUSH,
 * which was documented to use FREAD/FWRITE, continues to work.
 */
#define FREAD       0x0001
#define FWRITE      0x0002
#define O_NONBLOCK  0x0004      /* no delay */
#define O_APPEND    0x0008      /* set append mode */
#define O_SHLOCK    0x0010      /* open with shared file lock */
#define O_EXLOCK    0x0020      /* open with exclusive file lock */
#define O_ASYNC     0x0040      /* signal pgrp when data ready */
#define O_FSYNC     0x0080      /* synchronous writes */
#define O_CREAT     0x0200      /* create if nonexistant */
#define O_TRUNC     0x0400      /* truncate to zero length */
#define O_EXCL      0x0800      /* error if already exists */
#ifdef KERNEL
#define FMARK       0x1000      /* mark during gc() */
#define FDEFER      0x2000      /* defer for next gc pass */
#endif

/* defined by POSIX 1003.1; not 2.11BSD default, so bit is required */
/* Not currently implemented but it may be placed on the TODO list shortly */
#define O_NOCTTY    0x4000      /* don't assign controlling terminal */

#ifdef KERNEL
/* convert from open() flags to/from fflags; convert O_RD/WR to FREAD/FWRITE */
#define FFLAGS(oflags)  ((oflags) + 1)
#define OFLAGS(fflags)  ((fflags) - 1)

/* bits to save after open */
#define FMASK       (FREAD|FWRITE|FAPPEND|FASYNC|FFSYNC|FNONBLOCK)
/* bits settable by fcntl(F_SETFL, ...) */
#define FCNTLFLAGS  (FAPPEND|FASYNC|FFSYNC|FNONBLOCK)
#endif

/*
 * The O_* flags used to have only F* names, which were used in the kernel
 * and by fcntl.  We retain the F* names for the kernel f_flags field
 * and for backward compatibility for fcntl.
 */
#define FAPPEND     O_APPEND    /* kernel/compat */
#define FASYNC      O_ASYNC     /* kernel/compat */
#define FFSYNC      O_FSYNC     /* kernel */
#define FEXLOCK     O_EXLOCK    /* kernel */
#define FSHLOCK     O_SHLOCK    /* kernel */
#define FNONBLOCK   O_NONBLOCK  /* kernel */
#define FNDELAY     O_NONBLOCK  /* compat */
#define O_NDELAY    O_NONBLOCK  /* compat */

/*
 * Constants used for fcntl(2)
 */

/* command values */
#define F_DUPFD     0       /* duplicate file descriptor */
#define F_GETFD     1       /* get file descriptor flags */
#define F_SETFD     2       /* set file descriptor flags */
#define F_GETFL     3       /* get file status flags */
#define F_SETFL     4       /* set file status flags */
#define F_GETOWN    5       /* get SIGIO/SIGURG proc/pgrp */
#define F_SETOWN    6       /* set SIGIO/SIGURG proc/pgrp */

/* file descriptor flags (F_GETFD, F_SETFD) */
#define FD_CLOEXEC  1       /* close-on-exec flag */

/* lock operations for flock(2) */
#define LOCK_SH     0x01    /* shared file lock */
#define LOCK_EX     0x02    /* exclusive file lock */
#define LOCK_NB     0x04    /* don't block when locking */
#define LOCK_UN     0x08    /* unlock file */


#ifndef KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
int	open __P((const char *, int, ...));
int	creat __P((const char *, mode_t));
int	fcntl __P((int, int, ...));
#ifndef _POSIX_SOURCE
int	flock __P((int, int));
#endif /* !_POSIX_SOURCE */
__END_DECLS
#endif

#endif /* !_SYS_FCNTL_H_ */
