/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vmsystm.h	7.2.1 (2.11BSD GTE) 1/15/95
 */

#ifndef _SYS_VMSYSTM_H_
#define _SYS_VMSYSTM_H_

/*
 * Miscellaneous virtual memory subsystem variables and structures.
 */
extern size_t freemem;					/* remaining clicks of free memory */

#ifdef _KERNEL
extern int	avefree;					/* moving average of remaining free clicks */
extern int	avefree30;					/* 30 sec (avefree is 5 sec) moving average */

/* writable copies of tunables */
extern int	maxslp;						/* max sleep time before very swappable */
#endif

struct forkstat {
	long	cntfork;
	long	cntvfork;
	long	sizfork;
	long	sizvfork;
};

#ifdef _KERNEL
extern struct forkstat forkstat;
#endif
#endif /* _SYS_VMSYSTM_H_ */
