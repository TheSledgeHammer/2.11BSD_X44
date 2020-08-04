/*
 * sched.c
 *
 *  Created on: 12 Jul 2020
 *      Author: marti
 */


#include "sys/gsched.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/queue.h>
/*
 * 2.11BSD's cpu decay: determines priority, during schedcpu
 * modified 4.4BSD cpu decay: determines decay factor (calculated using 2.11BSD cpu decay)
 *
 * TODO: placement of p_pctcpu calculation from FSHIFT & CCPU_SHIFT
 */

/* decay 95% of `p_pctcpu' in 60 seconds; see CCPU_SHIFT before changing */
fixpt_t	ccpu = 		0.95122942450071400909 * FSCALE;		/* exp(-1/20) */
#define	CCPU_SHIFT	11

void
shift(p)
	register struct proc *p;
{
	/*
	 * p_pctcpu is only for ps.
	 */
#if	(FSHIFT >= CCPU_SHIFT)
	p->p_pctcpu += (hz == 100)?
		((fixpt_t) p->p_cpticks) << (FSHIFT - CCPU_SHIFT):
	        	100 * (((fixpt_t) p->p_cpticks)
			<< (FSHIFT - CCPU_SHIFT)) / hz;
#else
	p->p_pctcpu += ((FSCALE - ccpu) * (p->p_cpticks * FSCALE / hz)) >> FSHIFT;
#endif
	p->p_cpticks = 0;

}

/* TODO: Change where PPQ's are implemented/ calculated
 * -
 */
void
schedcpu(arg)
	void *arg;
{
	register struct proc *p;
	register int a;
	register u_char	currpri;

	wakeup((caddr_t)&lbolt);
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_time != 127)
			p->p_time++;
		/*
		 * this is where 2.11 does its real time alarms.  4.X uses
		 * timeouts, since it offers better than second resolution.
		 * Putting it here allows us to continue using use an int
		 * to store the number of ticks in the callout structure,
		 * since the kernel never has a timeout of greater than
		 * around 9 minutes.
		 */
		if (p->p_realtimer.it_value && !--p->p_realtimer.it_value) {
			psignal(p, SIGALRM);
			p->p_realtimer.it_value = p->p_realtimer.it_interval;
		}
		if (p->p_stat == SSLEEP || p->p_stat == SSTOP)
			if (p->p_slptime != 127)
				p->p_slptime++;
		if (p->p_slptime > 1)
			continue;
		a = (p->p_cpu & 0377) * SCHMAG + p->p_nice;
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		p->p_cpu = a;
		if (p->p_pri >= PUSER) {
			setpri(p);
		}
		/*
		 * sort by cpticks
		 * pass to edf
		 * test
		 * pass to cfs
		 * exit
		 */
	}

	vmmeter();
	if (runin != 0) {
		runin = 0;
		wakeup((caddr_t)&runin);
	}
	++runrun;			/* swtch at least once a second */
	timeout(schedcpu, (caddr_t)0, hz);
}
