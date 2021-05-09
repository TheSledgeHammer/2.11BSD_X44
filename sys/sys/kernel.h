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
extern long						hostid;
extern char						hostname[MAXHOSTNAMELEN];
extern int 						hostnamelen;

/* 1.2 */
extern volatile struct timeval 	mono_time;
extern struct timeval 			boottime;
extern struct timeval 			runtime;
extern volatile struct timeval 	time;
extern struct timezone 			tz;			/* XXX */
int								adjdelta;

extern int 						rtc_offset;	/* offset of rtc from UTC in minutes */

extern int						hard_ticks;	/* # of hardclock ticks */
extern int 						tick;		/* usec per tick (1000000 / hz) */
extern int						hz;			/* system clock's frequency */
extern int						mshz;		/* # milliseconds per hz */
extern int 						stathz;		/* statistics clock's frequency */
extern int 						profhz;		/* profiling clock's frequency */
extern int						lbolt;		/* awoken once a second */
extern int						psratio;	/* ratio: prof / stat */
int 							avenrun[3];

#endif /* _SYS_KERNEL_H_ */
