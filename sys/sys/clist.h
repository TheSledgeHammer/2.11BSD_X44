/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)clist.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * Raw structures for the character list routines.
 */
struct cblock {
	struct cblock   *c_next;			/* next cblock in queue */
	char 			c_quote[CBQSIZE];	/* quoted characters */
	char			c_info[CBSIZE];		/* characters */
};

#ifdef KERNEL
extern struct cblock *cfree, *cfreelist;
extern int cfreecount, nclist;
#endif
