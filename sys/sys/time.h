/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)time.h	8.5 (Berkeley) 5/4/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)time.h	1.3 (2.11BSD) 2000/4/21
 */

#ifndef	_SYS_TIME_H_
#define	_SYS_TIME_H_

#include <sys/types.h>

/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct timeval {
	time_t	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

/*
 * Structure defined by POSIX.4 to be like a timeval but with nanoseconds
 * instead of microseconds.  Silly on a PDP-11 but keeping the names the
 * same makes life simpler than changing the names.
*/
struct timespec {
	time_t	tv_sec;		/* seconds */
	long   	tv_nsec;	/* and nanoseconds */
};

#define	TIMEVAL_TO_TIMESPEC(tv, ts) {					\
	(ts)->tv_sec = (tv)->tv_sec;						\
	(ts)->tv_nsec = (tv)->tv_usec * 1000;				\
}
#define	TIMESPEC_TO_TIMEVAL(tv, ts) {					\
	(tv)->tv_sec = (ts)->tv_sec;						\
	(tv)->tv_usec = (ts)->tv_nsec / 1000;				\
}

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;		/* type of dst correction */
};

#define	DST_NONE	0	/* not on dst */
#define	DST_USA		1	/* USA style dst */
#define	DST_AUST	2	/* Australian style dst */
#define	DST_WET		3	/* Western European dst */
#define	DST_MET		4	/* Middle European dst */
#define	DST_EET		5	/* Eastern European dst */
#define	DST_CAN		6	/* Canada */

/*
 * Operations on timevals.
 *
 * NB: timercmp does not work for >= or <=.
 */
#define	timerclear(tvp)			((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define	timerisset(tvp)			((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)								\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				        \
	    ((tvp)->tv_usec cmp (uvp)->tv_usec) :			    \
	    ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define	timeradd(tvp, uvp, vvp)	do {						\
	(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;			\
	(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;		\
	if ((vvp)->tv_usec >= 1000000) {						\
		(vvp)->tv_sec++;									\
		(vvp)->tv_usec -= 1000000;							\
	}														\
} while (/* CONSTCOND */ 0)
#define	timersub(tvp, uvp, vvp) do {						\
	(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;			\
	(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;		\
	if ((vvp)->tv_usec < 0) {								\
		(vvp)->tv_sec--;									\
		(vvp)->tv_usec += 1000000;							\
	}														\
} while (/* CONSTCOND */ 0)

/* Operations on timespecs. */
#define	timespecclear(tsp)	(tsp)->tv_sec = (time_t)((tsp)->tv_nsec = 0L)
#define	timespecisset(tsp)	((tsp)->tv_sec || (tsp)->tv_nsec)
#define	timespeccmp(tsp, usp, cmp)							\
	(((tsp)->tv_sec == (usp)->tv_sec) ?						\
	    ((tsp)->tv_nsec cmp (usp)->tv_nsec) :				\
	    ((tsp)->tv_sec cmp (usp)->tv_sec))
#define	timespecadd(tsp, usp, vsp)							\
	do {													\
		(vsp)->tv_sec = (tsp)->tv_sec + (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec + (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec >= 1000000000L) {				\
			(vsp)->tv_sec++;								\
			(vsp)->tv_nsec -= 1000000000L;					\
		}													\
	} while (/* CONSTCOND */ 0)
#define	timespecsub(tsp, usp, vsp)							\
	do {													\
		(vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec < 0) {							\
			(vsp)->tv_sec--;								\
			(vsp)->tv_nsec += 1000000000L;					\
		}													\
	} while (/* CONSTCOND */ 0)
#define timespec2ns(x) (((uint64_t)(x)->tv_sec) * 1000000000L + (x)->tv_nsec)

/*
 * Names of the interval timers, and structure
 * defining a timer setting.
 */
#define	ITIMER_REAL		0
#define	ITIMER_VIRTUAL	1
#define	ITIMER_PROF		2

struct k_itimerval {
	long			it_interval;	/* timer interval */
	long			it_value;		/* current value */
};

struct itimerval {
	struct timeval 	it_interval;	/* timer interval */
	struct timeval 	it_value;		/* current value */
};

/*
 * Structure defined by POSIX.1b to be like a itimerval, but with
 * timespecs. Used in the timer_*() system calls.
 */
struct itimerspec {
	struct timespec it_interval;
	struct timespec it_value;
};

#define	CLOCK_REALTIME	0
#define	CLOCK_VIRTUAL	1
#define	CLOCK_PROF	2
#define	CLOCK_MONOTONIC	3

#define	TIMER_RELTIME	0x0	/* relative timer */
#define	TIMER_ABSTIME	0x1	/* absolute timer */

#define	TIMER_MAX	32

/*
 * Getkerninfo clock information structure
 */
struct clockinfo {
	int	hz;			/* clock frequency */
	int	tick;		/* micro-seconds per hz tick */
	int	stathz;		/* statistics clock frequency */
	int	profhz;		/* profiling clock frequency */
};

/*
 * hide bintime for _STANDALONE because this header is used for hpcboot.exe,
 * which is built with compilers which don't recognize LL suffix.
 *	http://mail-index.NetBSD.org/tech-userlevel/2008/02/27/msg000181.html
 */
#if !defined(_STANDALONE)
struct bintime {
	time_t	sec;
	uint64_t frac;
};

static __inline void
bintime_addx(struct bintime *bt, uint64_t x)
{
	uint64_t u;

	u = bt->frac;
	bt->frac += x;
	if (u > bt->frac)
		bt->sec++;
}

static __inline void
bintime_add(struct bintime *bt, const struct bintime *bt2)
{
	uint64_t u;

	u = bt->frac;
	bt->frac += bt2->frac;
	if (u > bt->frac)
		bt->sec++;
	bt->sec += bt2->sec;
}

static __inline void
bintime_sub(struct bintime *bt, const struct bintime *bt2)
{
	uint64_t u;

	u = bt->frac;
	bt->frac -= bt2->frac;
	if (u < bt->frac)
		bt->sec--;
	bt->sec -= bt2->sec;
}

#define	bintimecmp(bta, btb, cmp)					\
	(((bta)->sec == (btb)->sec) ?					\
	    ((bta)->frac cmp (btb)->frac) :				\
	    ((bta)->sec cmp (btb)->sec))

/*-
 * Background information:
 *
 * When converting between timestamps on parallel timescales of differing
 * resolutions it is historical and scientific practice to round down rather
 * than doing 4/5 rounding.
 *
 *   The date changes at midnight, not at noon.
 *
 *   Even at 15:59:59.999999999 it's not four'o'clock.
 *
 *   time_second ticks after N.999999999 not after N.4999999999
 */

/*
 * The magic numbers for converting ms/us/ns to fractions
 */

/* 1ms = (2^64) / 1000       */
#define	BINTIME_SCALE_MS	((uint64_t)18446744073709551ULL)

/* 1us = (2^64) / 1000000    */
#define	BINTIME_SCALE_US	((uint64_t)18446744073709ULL)

/* 1ns = (2^64) / 1000000000 */
#define	BINTIME_SCALE_NS	((uint64_t)18446744073ULL)

static __inline void
bintime2timespec(const struct bintime *bt, struct timespec *ts)
{

	ts->tv_sec = bt->sec;
	ts->tv_nsec =
	    (long)((1000000000ULL * (uint32_t)(bt->frac >> 32)) >> 32);
}

static __inline void
timespec2bintime(const struct timespec *ts, struct bintime *bt)
{

	bt->sec = ts->tv_sec;
	bt->frac = (uint64_t)ts->tv_nsec * BINTIME_SCALE_NS;
}

static __inline void
bintime2timeval(const struct bintime *bt, struct timeval *tv)
{

	tv->tv_sec = bt->sec;
	tv->tv_usec =
	    (int)((1000000ULL * (uint32_t)(bt->frac >> 32)) >> 32);
}

static __inline void
timeval2bintime(const struct timeval *tv, struct bintime *bt)
{

	bt->sec = tv->tv_sec;
	bt->frac = (uint64_t)tv->tv_usec * BINTIME_SCALE_US;
}

static __inline struct bintime
ms2bintime(uint64_t ms)
{
	struct bintime bt;

	bt.sec = (time_t)(ms / 1000U);
	bt.frac = (uint64_t)(ms % 1000U) * BINTIME_SCALE_MS;

	return bt;
}

static __inline struct bintime
us2bintime(uint64_t us)
{
	struct bintime bt;

	bt.sec = (time_t)(us / 1000000U);
	bt.frac = (uint64_t)(us % 1000000U) * BINTIME_SCALE_US;

	return bt;
}

static __inline struct bintime
ns2bintime(uint64_t ns)
{
	struct bintime bt;

	bt.sec = (time_t)(ns / 1000000000U);
	bt.frac = (uint64_t)(ns % 1000000000U) * BINTIME_SCALE_NS;

	return bt;
}
#endif /* !defined(_STANDALONE) */

#ifdef _KERNEL  /*|| STANDALONE*/
extern volatile time_t	time_second;
extern volatile time_t  time_uptime;
#endif

#ifdef _KERNEL
void	binuptime(struct bintime *);
void	nanouptime(struct timespec *);
void	microuptime(struct timeval *);
void	bintime(struct bintime *);
void	nanotime(struct timespec *);
void	microtime(struct timeval *);
void	getbinuptime(struct bintime *);
void	getnanouptime(struct timespec *);
void	getmicrouptime(struct timeval *);
void	getbintime(struct bintime *);
void	getnanotime(struct timespec *);
void	getmicrotime(struct timeval *);
void	getbinboottime(struct bintime *);
void	getnanoboottime(struct timespec *);
void	getmicroboottime(struct timeval *);
void	inittimecounter(void);
int		itimerdecr(struct itimerval *itp,int usec);
int		itimerfix(struct timeval *);
void	timevaladd(struct timeval *, struct timeval *);
void	timevalfix(struct timeval *);
void	timevalsub(struct timeval *, struct timeval *);
int 	ratecheck(struct timeval *, const struct timeval *);
int		ppsratecheck(struct timeval *, int *, int);
int 	tvtohz(const struct timeval *);
#else /* !KERNEL */

#include <sys/cdefs.h>
#include <time.h>

#if (_POSIX_C_SOURCE - 0) >= 200112L || \
    defined(_XOPEN_SOURCE) || defined(__BSD_VISIBLE)

__BEGIN_DECLS
int	adjtime(const struct timeval *, struct timeval *);
int	getitimer(int, struct itimerval *);
int	gettimeofday(struct timeval *, struct timezone *);
int	setitimer(int, const struct itimerval *, struct itimerval *);
int	settimeofday(const struct timeval *, const struct timezone *);
int	utimes(const char *, const struct timeval *);
__END_DECLS

#endif /* !__BSD_VISIBLE */
#endif /* !KERNEL */
#endif	/* !_SYS_TIME_H_ */
