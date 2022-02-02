/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vmsystm.h	7.2.1 (2.11BSD GTE) 1/15/95
 */

#ifndef _VM_SYSTM_H_
#define _VM_SYSTM_H_

#include <sys/vmmeter.h>

/*
 * Miscellaneous virtual memory subsystem variables and structures.
 */
#if defined(KERNEL)
int	freemem = cnt.v_free_count;	/* remaining clicks of free memory */

int	avefree;					/* moving average of remaining free clicks */
int	avefree30;					/* 30 sec (avefree is 5 sec) moving average */

/* writable copies of tunables */
int	maxslp;						/* max sleep time before very swappable */
#endif

struct forkstat {
	long	cntfork;
	long	cntvfork;
	long	sizfork;
	long	sizvfork;
};

#if defined(KERNEL)
struct forkstat forkstat;
#endif
#endif /* _VM_SYSTM_H_ */
