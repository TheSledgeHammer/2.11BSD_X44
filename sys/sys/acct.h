/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)acct.h	2.2	(2.11BSD) 1999/2/19
 */
#ifndef _SYS_ACCT_H_
#define _SYS_ACCT_H_

/*
 * Accounting structures;
 * these use a comp_t type which is a 3 bits base 8
 * exponent, 13 bit fraction ``floating point'' number.
 * Units are 1/AHZ seconds.
 */
typedef	u_short comp_t;

struct acct {
	char	ac_comm[10];	/* Accounting command name */
	comp_t	ac_utime;		/* Accounting user time */
	comp_t	ac_stime;		/* Accounting system time */
	comp_t	ac_etime;		/* Accounting elapsed time */
	time_t	ac_btime;		/* Beginning time */
	uid_t	ac_uid;			/* Accounting user ID */
	gid_t	ac_gid;			/* Accounting group ID */
	short	ac_mem;			/* average memory usage */
	comp_t	ac_io;			/* number of disk IO blocks */
	dev_t	ac_tty;			/* control typewriter */
	char	ac_flag;		/* Accounting flag */
};

#define	AFORK	0001		/* has executed fork, but no exec */
#define	ASU		0002		/* used super-user privileges */
#define	ACOMPAT	0004		/* used compatibility mode */
#define	ACORE	0010		/* dumped core */
#define	AXSIG	0020		/* killed by a signal */
#define	ASUGID	0040		/* setuser/group id privileges used */

/*
 * 1/AHZ is the granularity of the data encoded in the various
 * comp_t fields.  This is not necessarily equal to hz.
 */
#define AHZ 64

#ifndef	_KERNEL
struct vnode	*acctp;

#define	_PATH_ACCTD		"/usr/libexec/acctd"
#define	_PATH_ACCTFILE	"/usr/adm/acct"
#define	_PATH_ACCTDPID 	"/var/run/acctd.pid"
#define	_PATH_ACCTDCF 	"/etc/acctd.cf"
#define	_PATH_DEVALOG 	"/dev/acctlog"
#endif
#ifdef	_KERNEL
int	acct_process(struct proc *);
#endif

#endif	/* !_SYS_ACCT_H_ */
