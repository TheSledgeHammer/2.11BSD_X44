/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)kern_time.c	8.4 (Berkeley) 5/26/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_time.c	1.5 (2.11BSD) 2000/4/9
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/sysdecl.h>

static void setthetime(struct timeval);

/* 
 * Time of day and interval timer support.
 *
 * These routines provide the kernel entry points to get and set
 * the time-of-day.
 */
int
gettimeofday()
{
	register struct gettimeofday_args {
		syscallarg(struct timeval *) tp;
		syscallarg(struct timezone *) tzp;
	} *uap = (struct gettimeofday_args *)u.u_ap;
	struct timeval atv;

	int s;
	register u_int	ms;

	if (SCARG(uap, tp)) {
		/*
		 * We don't resolve the milliseconds on every clock tick; it's
		 * easier to do it here.  Long casts are out of paranoia.
		 */
		s = splhigh();
		atv = time;
		ms = lbolt;
		splx(s);
		atv.tv_usec = (long)ms * mshz;
		u.u_error = copyout((caddr_t)&atv, (caddr_t)(SCARG(uap, tp)), sizeof(atv));
		if (u.u_error)
			return (u.u_error);
	}
	if (SCARG(uap, tzp))
		u.u_error = copyout((caddr_t)&tz, (caddr_t)SCARG(uap, tzp), sizeof (tz));
	return (u.u_error);
}

int
settimeofday()
{
	register struct settimeofday_args {
		syscallarg(struct timeval *) tv;
		syscallarg(struct timezone *) tzp;
	} *uap = (struct settimeofday_args *)u.u_ap;
	struct timeval atv;
	struct timezone atz;

	if (SCARG(uap, tv)) {
		u.u_error = copyin((caddr_t)SCARG(uap, tv), (caddr_t)&atv, sizeof(struct timeval));
		if (u.u_error)
			return (u.u_error);
		setthetime(atv);
		if	(u.u_error)
			return (u.u_error);
	}
	if (SCARG(uap, tzp) && suser()) {
		u.u_error = copyin((caddr_t)SCARG(uap, tzp), (caddr_t)&atz, sizeof(atz));
		if (u.u_error == 0) {
			tz = atz;
		} else {
			return (u.u_error);
		}
	}

	return (0);
}

static void
setthetime(tv)
	register struct timeval tv;
{
    struct timeval delta;
	int	s;

	if (!suser())
		return;
#ifdef	NOTNOW
	/*
	 * If the system is secure, we do not allow the time to be set to an
	 * earlier value.  The time may be slowed (using adjtime) but not set back.
	 *
	 * NOTE:  Can not do this until ntpd is updated to deal with the coarse (50, 60
	 *	  hz) clocks.  Ntpd wants to adjust time system clock a few microseconds
	 *	  at a time (which gets rounded to 0 in adjtime below). If that fails
	 *	  ntpd uses settimeofday to step the time backwards which obviously
	 *	  will fail if the next 'if' is enabled - all that does is fill up the
	 *	  logfiles with "can't set time" messages and the time keeps drifting.
	 */
	if (securelevel > 0 && timercmp(&tv, &time, <)) {
		u.u_error = EPERM; /* XXX */
		return;
	}
#endif
	/* WHAT DO WE DO ABOUT PENDING REAL-TIME TIMEOUTS??? */
	s = splclock();
	delta.tv_sec = tv.tv_sec - time.tv_sec;
	delta.tv_usec = tv.tv_usec - time.tv_usec;
	time = tv;
	(void) splsoftclock();
	timevaladd(&boottime, &delta);
	timevalfix(&boottime);
	timevaladd(&runtime, &delta);
	timevalfix(&runtime);
	splx(s);
	resettodr();
}

int
adjtime()
{
	register struct adjtime_args {
		syscallarg(struct timeval *) delta;
		syscallarg(struct timeval *) olddelta;
	} *uap = (struct adjtime_args *)u.u_ap;
	struct timeval atv;
	register int s;
	register long adjust;

	if (u.u_error == suser()) {
		return (u.u_error);
	}
	u.u_error = copyin((caddr_t)SCARG(uap, delta), (caddr_t)&atv, sizeof(struct timeval));
	if (u.u_error)
		return (u.u_error);
	adjust = (atv.tv_sec * hz) + (atv.tv_usec / mshz);
	/* if unstoreable values, just set the clock */
	if (adjust > 0x7fff || adjust < 0x8000) {
		s = splclock();
		time.tv_sec += atv.tv_sec;
		lbolt += atv.tv_usec / mshz;
		while (lbolt >= hz) {
			lbolt -= hz;
			++time.tv_sec;
		}
		splx(s);
		if (!SCARG(uap, olddelta))
			return (0);
		atv.tv_sec = atv.tv_usec = 0;
	} else {
		if (!SCARG(uap, olddelta)) {
			adjdelta = adjust;
			return (0);
		}
		atv.tv_sec = adjdelta / hz;
		atv.tv_usec = (adjdelta % hz) * mshz;
		adjdelta = adjust;
	}
	(void) copyout((caddr_t)&atv, (caddr_t)SCARG(uap, olddelta), sizeof(struct timeval));
	return (0);
}

int
getitimer()
{
	register struct getitimer_args {
		syscallarg(u_int) which;
		syscallarg(struct itimerval *) itv;
	} *uap = (struct getitimer_args *)u.u_ap;
    struct itimerval aitv;
	register int s;

	if (SCARG(uap, which) > ITIMER_PROF) {
		u.u_error = EINVAL;
		return (u.u_error);
	}
	aitv.it_interval.tv_usec = 0;
	aitv.it_value.tv_usec = 0;
	s = splclock();
	if (SCARG(uap, which) == ITIMER_REAL) {
		register struct proc *p = u.u_procp;
		
		aitv.it_interval.tv_sec = p->p_realtimer.it_interval;
		aitv.it_value.tv_sec = p->p_realtimer.it_value;

		if(timerisset(&aitv.it_value)) {
		    if (timercmp(&aitv.it_value, &time, <)) {
		        timerclear(&aitv.it_value);
		    } else {
		        timevalsub(&aitv.it_value, (struct timeval *)&time);
		    }
		} 
	} else {
		register struct k_itimerval *t = &u.u_timer[SCARG(uap, which) - 1];

		aitv.it_interval.tv_sec = t->it_interval / hz;
		aitv.it_value.tv_sec = t->it_value / hz;
	}
	splx(s);
	u.u_error = copyout((caddr_t)&aitv, (caddr_t)SCARG(uap, itv), sizeof(struct itimerval));
	return (u.u_error);
}

int
setitimer()
{
	register struct setitimer_args {
		syscallarg(u_int) which;
		syscallarg(struct itimerval *) itv;
		syscallarg(struct itimerval *) oitv;
	} *uap = (struct setitimer_args *)u.u_ap;

	struct itimerval aitv;
	register struct itimerval *aitvp;
	int s;

	if (SCARG(uap, which) > ITIMER_PROF) {
		u.u_error = EINVAL;
		return (EINVAL);
	}
	aitvp = SCARG(uap, itv);
	if (SCARG(uap, oitv)) {
		SCARG(uap, itv) = SCARG(uap, oitv);
		getitimer();
	}
	if (aitvp == 0)
		return (0);
	u.u_error = copyin((caddr_t)aitvp, (caddr_t)&aitv, sizeof(struct itimerval));
	if (u.u_error)
		return (u.u_error);
	s = splclock();
	if (SCARG(uap, which) == ITIMER_REAL) {
		register struct proc *p = u.u_procp;

		p->p_realtimer.it_value = aitv.it_value.tv_sec;
		if (aitv.it_value.tv_usec)
			++p->p_realtimer.it_value;
		p->p_realtimer.it_interval = aitv.it_interval.tv_sec;
		if (aitv.it_interval.tv_usec)
			++p->p_realtimer.it_interval;
	} else {
		register struct k_itimerval *t = &u.u_timer[SCARG(uap, which) - 1];

		t->it_value = aitv.it_value.tv_sec * hz;
		if (aitv.it_value.tv_usec)
			t->it_value += hz;
		t->it_interval = aitv.it_interval.tv_sec * hz;
		if (aitv.it_interval.tv_usec)
			t->it_interval += hz;
	}
	splx(s);
	return (0);
}

void
realitexpire(arg)
	void *arg;
{
	register struct proc *p;
	int s;
	
	p = (struct proc *)arg;
	if(p == NULL) {
		p = u.u_procp;
	}
	psignal(p, SIGALRM);
	if (!timerisset(&p->p_krealtimer.it_interval)) {
		timerclear(&p->p_krealtimer.it_value);
		return;
	}
	for (;;) {
		s = splclock();
		timevaladd(&p->p_krealtimer.it_value, &p->p_krealtimer.it_interval);
		if (timercmp(&p->p_krealtimer.it_value, &time, >)) {
			timeout(realitexpire, (caddr_t)p, hzto(&p->p_krealtimer.it_value));
			splx(s);
			return;
		}
		splx(s);
	}
}

/*
 * Check that a proposed value to load into the .it_value or
 * .it_interval part of an interval timer is acceptable, and
 * fix it to have at least minimal value (i.e. if it is less
 * than the resolution of the clock, round it up.)
 */
int
itimerfix(tv)
	struct timeval *tv;
{
	if (tv->tv_sec < 0 || tv->tv_sec > 100000000L ||
	    tv->tv_usec < 0 || tv->tv_usec >= 1000000L)
		return (EINVAL);
	if (tv->tv_sec == 0 && tv->tv_usec != 0 && tv->tv_usec < (1000/hz))
		tv->tv_usec = 1000/hz;
	return (0);
}

/*
 * Decrement an interval timer by a specified number
 * of microseconds, which must be less than a second,
 * i.e. < 1000000.  If the timer expires, then reload
 * it.  In this case, carry over (usec - old value) to
 * reducint the value reloaded into the timer so that
 * the timer does not drift.  This routine assumes
 * that it is called in a context where the timers
 * on which it is operating cannot change in value.
 */
int
itimerdecr(itp, usec)
	register struct itimerval *itp;
	int usec;
{
	if (itp->it_value.tv_usec < usec) {
		if (itp->it_value.tv_sec == 0) {
			/* expired, and already in next interval */
			usec -= itp->it_value.tv_usec;
			goto expire;
		}
		itp->it_value.tv_usec += 1000000L;
		itp->it_value.tv_sec--;
	}
	itp->it_value.tv_usec -= usec;
	usec = 0;
	if (timerisset(&itp->it_value))
		return (1);
	/* expired, exactly at end of interval */
expire:
	if (timerisset(&itp->it_interval)) {
		itp->it_value = itp->it_interval;
		itp->it_value.tv_usec -= usec;
		if (itp->it_value.tv_usec < 0) {
			itp->it_value.tv_usec += 1000000L;
			itp->it_value.tv_sec--;
		}
	} else
		itp->it_value.tv_usec = 0;		/* sec is already 0 */
	return (0);
}

/*
 * Add and subtract routines for timevals.
 * N.B.: subtract routine doesn't deal with
 * results which are before the beginning,
 * it just gets very confused in this case.
 * Caveat emptor.
 */
void
timevaladd(t1, t2)
	struct timeval *t1, *t2;
{
	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	timevalfix(t1);
}

void
timevalsub(t1, t2)
	struct timeval *t1, *t2;
{
	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

void
timevalfix(t1)
	struct timeval *t1;
{
	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000L;
	}
	if (t1->tv_usec >= 1000000L) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000L;
	}
}

/*
 * Compute number of ticks in the specified amount of time.
 */
int
tvtohz(tv)
	const struct timeval *tv;
{
	unsigned long ticks;
	long sec, usec;

	/*
	 * If the number of usecs in the whole seconds part of the time
	 * difference fits in a long, then the total number of usecs will
	 * fit in an unsigned long.  Compute the total and convert it to
	 * ticks, rounding up and adding 1 to allow for the current tick
	 * to expire.  Rounding also depends on unsigned long arithmetic
	 * to avoid overflow.
	 *
	 * Otherwise, if the number of ticks in the whole seconds part of
	 * the time difference fits in a long, then convert the parts to
	 * ticks separately and add, using similar rounding methods and
	 * overflow avoidance.  This method would work in the previous
	 * case, but it is slightly slower and assumes that hz is integral.
	 *
	 * Otherwise, round the time difference down to the maximum
	 * representable value.
	 *
	 * If ints are 32-bit, then the maximum value for any timeout in
	 * 10ms ticks is 248 days.
	 */
	sec = tv->tv_sec;
	usec = tv->tv_usec;

	KASSERT(usec >= 0);
	KASSERT(usec < 1000000);

	/* catch overflows in conversion time_t->int */
	if (tv->tv_sec > INT_MAX) {
		return INT_MAX;
	}
	if (tv->tv_sec < 0) {
		return 0;
	}

	if (sec < 0 || (sec == 0 && usec == 0)) {
		/*
		 * Would expire now or in the past.  Return 0 ticks.
		 * This is different from the legacy tvhzto() interface,
		 * and callers need to check for it.
		 */
		ticks = 0;
	} else if (sec <= (LONG_MAX / 1000000)) {
		ticks = (((sec * 1000000) + (unsigned long)usec + (tick - 1))
		    / tick) + 1;
	} else if (sec <= (LONG_MAX / hz)) {
		ticks = (sec * hz) +
		    (((unsigned long)usec + (tick - 1)) / tick) + 1;
	} else {
		ticks = LONG_MAX;
	}

	if (ticks > INT_MAX) {
		ticks = INT_MAX;
	}

	return ((int)ticks);
}

/* TODO: Merge settime and setthetime */
static int
settime(tv)
    struct timeval *tv;
{
    if (tv->tv_sec > UINT_MAX - 365*24*60*60) {
        return (EPERM);
    }
/*
    if (securelevel > 1 && timercmp(&tv, &time, <)) {
        return (EPERM);
    }
*/
    setthetime(*tv);
    return (0);
}

int
clock_gettime()
{
	register struct clock_gettime_args {
		syscallarg(clockid_t) clock_id;
		syscallarg(struct timespec *) tp;
	} *uap = (struct clock_gettime_args *)u.u_ap;
	struct timeval atv;
	struct timespec ats;
	int s;

	switch (SCARG(uap, clock_id)) {
	case CLOCK_REALTIME:
		microtime(&atv);
		TIMEVAL_TO_TIMESPEC(&atv, &ats);
		break;
	case CLOCK_MONOTONIC:
		s = splclock();
		atv = mono_time;
		splx(s);
		TIMEVAL_TO_TIMESPEC(&atv, &ats);
		break;
	default:
		u.u_error = EINVAL;
		return (EINVAL);
	}

	u.u_error = copyout(&ats, SCARG(uap, tp), sizeof(ats));
	return (u.u_error);
}

int
clock_settime()
{
	register struct clock_settime_args {
		syscallarg(clockid_t) clock_id;
		syscallarg(const struct timespec *) tp;
	} *uap = (struct clock_settime_args *)u.u_ap;
	struct timespec ats;
	struct timeval atv;

	u.u_error = suser();
	if (u.u_error != 0) {
		return (u.u_error);
	}

	u.u_error = copyin((caddr_t)SCARG(uap, tp), (caddr_t)&ats, sizeof(struct timespec));
	if (u.u_error != 0) {
		return (u.u_error);
	}
	switch (SCARG(uap, clock_id)) {
	case CLOCK_REALTIME:
		TIMESPEC_TO_TIMEVAL(&atv, &ats);
		u.u_error = settime(&atv);
		if (u.u_error != 0) {
			return (u.u_error);
		}
		break;
	case CLOCK_MONOTONIC:
		u.u_error = EINVAL;
		return (EINVAL);
	default:
		u.u_error = EINVAL;
		return (EINVAL);
	}

	return (0);
}

int
nanosleep()
{
	register struct nanosleep_args {
		syscallarg(struct timespec *) rqtp;
		syscallarg(struct timespec *) rmtp;
	} *uap = (struct nanosleep_args*) u.u_ap;
	int nanowait;
	struct timespec rqt;
	struct timespec rmt;
	struct timeval atv, utv;
	int s, timo;

	u.u_error = copyin((caddr_t)SCARG(uap, rqtp), (caddr_t)&rqt, sizeof(struct timespec));
	if (u.u_error) {
		return (u.u_error);
	}
	TIMESPEC_TO_TIMEVAL(&atv, &rqt);
	if (itimerfix(&atv)) {
		u.u_error = EINVAL;
		return (u.u_error);
	}

	s = splclock();
	timeradd(&atv, &time, &atv);
	timo = hzto(&atv);
	/*
	 * Avoid inadvertantly sleeping forever
	 */
	if (timo == 0) {
		timo = 1;
	}
	splx(s);

	u.u_error = tsleep(&nanowait, PWAIT | PCATCH, "nanosleep", timo);
	if (u.u_error == ERESTART) {
		u.u_error = EINTR;
	}
	if (u.u_error == EWOULDBLOCK) {
		u.u_error = 0;
	}

	if (SCARG(uap, rmtp)) {
		s = splclock();
		utv = time;
		splx(s);

		timersub(&atv, &utv, &utv);
		if (utv.tv_sec < 0) {
			timerclear(&utv);
		}

		TIMEVAL_TO_TIMESPEC(&utv, &rmt);
		u.u_error = copyout((caddr_t)&rmt, (caddr_t)SCARG(uap, rmtp), sizeof(rmt));
		if (u.u_error) {
			return (u.u_error);
		}
	}

	return (u.u_error);
}
