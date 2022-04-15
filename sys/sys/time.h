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
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

/*
 * Structure defined by POSIX.4 to be like a timeval but with nanoseconds
 * instead of microseconds.  Silly on a PDP-11 but keeping the names the
 * same makes life simpler than changing the names.
*/
struct timespec {
	long	tv_sec;		/* seconds */
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
	((tvp)->tv_sec cmp (uvp)->tv_sec || 					\
	(tvp)->tv_sec == (uvp)->tv_sec && 						\
	(tvp)->tv_usec cmp (uvp)->tv_usec)

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
 * Getkerninfo clock information structure
 */
struct clockinfo {
	int	hz;			/* clock frequency */
	int	tick;		/* micro-seconds per hz tick */
	int	stathz;		/* statistics clock frequency */
	int	profhz;		/* profiling clock frequency */
};

#ifdef _KERNEL  /*|| STANDALONE*/
extern volatile time_t	time_second;
#endif

#ifdef _KERNEL
int		itimerdecr(struct itimerval *itp,int usec);
int		itimerfix(struct timeval *);
void	microtime(struct timeval *);
void	timevaladd(struct timeval *, struct timeval *);
void	timevalfix(struct timeval *);
void	timevalsub(struct timeval *, struct timeval *);
#else /* !KERNEL */

#include <time.h>

#ifndef _POSIX_SOURCE
#include <sys/cdefs.h>

__BEGIN_DECLS
int	adjtime(const struct timeval *, struct timeval *);
int	getitimer(int, struct itimerval *);
int	gettimeofday(struct timeval *, struct timezone *);
int	setitimer(int, const struct itimerval *, struct itimerval *);
int	settimeofday(const struct timeval *, const struct timezone *);
int	utimes(const char *, const struct timeval *);
__END_DECLS
#endif /* !POSIX */
#endif /* !KERNEL */
#endif	/* !_SYS_TIME_H_ */
