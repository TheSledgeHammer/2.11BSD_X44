/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kernel.h	1.3 (2.11BSD GTE) 1997/2/14
 */
#ifndef _SYS_KERNEL_H_
#define _SYS_KERNEL_H_
/*
 * Global variables for the kernel
 */

/* 1.1 */
extern long	hostid;
extern char	hostname[MAXHOSTNAMELEN];
extern int hostnamelen;

/* 1.2 */
extern struct timeval boottime;
extern struct timeval runtime;
extern volatile struct timeval time;
extern struct timezone tz;			/* XXX */
int	adjdelta;

extern int 	tick;		/* usec per tick (1000000 / hz) */
extern int	hz;			/* system clock's frequency */
extern int	mshz;		/* # milliseconds per hz */
extern int 	stathz;		/* statistics clock's frequency */
extern int 	profhz;		/* profiling clock's frequency */
extern int	lbolt;		/* awoken once a second */

int	realitexpire();

short	avenrun[3];

/*
 * The following macros are used to declare global sets of objects, which
 * are collected by the linker into a `struct linker_set' as defined below.
 *
 * NB: the constants defined below must match those defined in
 * ld/ld.h.  Since their calculation requires arithmetic, we
 * can't name them symbolically (e.g., 23 is N_SETT | N_EXT).
 */
#define MAKE_SET(set, sym, type) \
	asm(".stabs \"_" #set "\", " #type ", 0, 0, _" #sym)
#define TEXT_SET(set, sym) MAKE_SET(set, sym, 23)
#define DATA_SET(set, sym) MAKE_SET(set, sym, 25)
#define BSS_SET(set, sym)  MAKE_SET(set, sym, 27)
#define ABS_SET(set, sym)  MAKE_SET(set, sym, 21)

#define PSEUDO_SET(sym)	   TEXT_SET(pseudo_set, sym)

struct linker_set {
	int ls_length;
	caddr_t ls_items[1];	/* really ls_length of them, trailing NULL */
};

#endif
