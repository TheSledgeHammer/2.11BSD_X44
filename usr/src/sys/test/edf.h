/*
 * edf.h
 *
 *  Created on: 17 Oct 2019
 *      Author: marti
 */

#ifndef _EDF_H_
#define _EDF_H_

#include <../sys/proc.h>
#include <../sys/types.h>
#include <../sys/time.h>

enum {
	Maxsteps = 200 * 100 * 2,	/* 100 periods of 200 procs */

	/* Edf.flags field */
	Admitted		= 0x01,
	Sporadic		= 0x02,
	Yieldonblock	= 0x04,
	Sendnotes		= 0x08,
	Deadline		= 0x10,
	Yield			= 0x20,
	Extratime		= 0x40,

	Infinity 		= ~0ULL,
};

typedef struct Edf	Edf;

struct Edf {
	/* All times in µs */
	/* time intervals */
	long			D;			/* Deadline */
	long			Delta;		/* Inherited deadline */
	long			T;			/* period */
	long			C;			/* Cost */
	long			S;			/* Slice: time remaining in this period */
								/* times (only low-order bits of absolute time) */
	long			r;			/* (this) release time */
	long			d;			/* (this) deadline */
	long			t;			/* Start of next period, t += T at release */
	long			s;			/* Time at which this proc was last scheduled */
								/* for schedulability testing */
	long			testDelta;
	int		    	testtype;	/* Release or Deadline */
	long			testtime;
	struct proc		*testnext;
								/* other */
	ushort			flags;
	struct timeval;
								/* Stats */
	long			edfused;
	long			extraused;
	long			aged;
	u_long			periods;
	u_long			missed;
};

#endif /* _EDF_H_ */
