/*
 * Program: sleep.c
 * Copyright: 1997, sms
 * Author: Steven M. Schultz
 *
 * Version   Date	Modification
 *     1.0  1997/9/26	1. Initial release.
*/

#include <stdio.h>	/* For NULL */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <unistd.h>

/*
 * This implements the usleep(3) function using only 1 system call (select)
 * instead of the 9 that the old implementation required.  Also this version 
 * avoids using signals (with the attendant system overhead).
 *
 * Nothing is returned and if less than ~20000 microseconds is specified the
 * select will return without any delay at all.
*/
void
usleep(micros)
	long micros;
{
	struct timeval s;

	if (micros > 0) {
		s.tv_sec = micros / 1000000L;
		s.tv_usec = micros % 1000000L;
		select(0, NULL, NULL, NULL, &s);
	}
}


#define	TICK	10000		/* system clock resolution in microseconds */
#define	USPS	1000000		/* number of microseconds in a second */
/*
#define	setvec(vec, a) \
	vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0

static int ringring;

void
usleep(useconds)
	unsigned int useconds;
{
	register struct itimerval *itp;
	struct itimerval itv, oitv;
	struct sigvec vec, ovec;
	long omask;
	static void sleephandler();

	itp = &itv;
	if (!useconds)
		return;
	timerclear(&itp->it_interval);
	timerclear(&itp->it_value);
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return;
	itp->it_value.tv_sec = useconds / USPS;
	itp->it_value.tv_usec = useconds % USPS;
	if (timerisset(&oitv.it_value)) {
		if (timercmp(&oitv.it_value, &itp->it_value, >)) {
			oitv.it_value.tv_sec -= itp->it_value.tv_sec;
			oitv.it_value.tv_usec -= itp->it_value.tv_usec;
			if (oitv.it_value.tv_usec < 0) {
				oitv.it_value.tv_usec += USPS;
				oitv.it_value.tv_sec--;
			}
		} else {
			itp->it_value = oitv.it_value;
			oitv.it_value.tv_sec = 0;
			oitv.it_value.tv_usec = 2 * TICK;
		}
	}
	setvec(vec, sleephandler);
	(void) sigvec(SIGALRM, &vec, &ovec);
	omask = sigblock(sigmask(SIGALRM));
	ringring = 0;
	(void) setitimer(ITIMER_REAL, itp, (struct itimerval*) 0);
	while (!ringring)
		sigpause(omask & ~sigmask(SIGALRM));
	(void) sigvec(SIGALRM, &ovec, (struct sigvec*) 0);
	(void) sigsetmask(omask);
	(void) setitimer(ITIMER_REAL, &oitv, (struct itimerval*) 0);
}

static void
sleephandler()
{
	ringring = 1;
}
*/
