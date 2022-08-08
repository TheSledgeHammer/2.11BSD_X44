/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vmsystm.h	7.2.1 (2.11BSD GTE) 1/15/95
 */

#ifndef _VM_SYSTM_H_
#define _VM_SYSTM_H_

/*
 * Miscellaneous virtual memory subsystem variables and structures.
 */
size_t freemem;							/* remaining clicks of free memory */

#ifdef _KERNEL
int	avefree;							/* moving average of remaining free clicks */
int	avefree30;							/* 30 sec (avefree is 5 sec) moving average */

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
struct forkstat forkstat;
#endif
#endif /* _VM_SYSTM_H_ */
