
/* Source here is to eventually be merged with kern_synch.c */

#include "sys/gsched.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/queue.h>

/* decay 95% of `p_pctcpu' in 60 seconds; see CCPU_SHIFT before changing */
fixpt_t	ccpu = 		0.95122942450071400909 * FSCALE;		/* exp(-1/20) */
#define	CCPU_SHIFT	11

void
ccpu_shift(p)
	register struct proc *p;
{
	register int s;

	s = splstatclock();	/* prevent state changes */
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
		p->p_swtime++;
		if (p->p_stat == SSLEEP || p->p_stat == SSTOP)
			if (p->p_slptime != 127)
				p->p_slptime++;
		p->p_pctcpu = (p->p_pctcpu * ccpu) >> FSHIFT;
		if (p->p_slptime > 1)
			continue;
		ccpu_shift(p);
		a = (p->p_cpu & 0377) * SCHMAG + p->p_nice;
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		p->p_cpu = a;
		if (p->p_pri >= PUSER) {
			setpri(p);
		}
		if(edf_schedcpu(p)) {
			if((p != curproc) && p->p_stat == SRUN && (p->p_flag & P_INMEM) && (p->p_pri / PPQ) != (currpri / PPQ)) {
				if(cfs_schedcpu(p)) {
					continue;
				}
				p->p_pri = currpri;
				setpri(p);
			} else {
				setpri(p);
			}
		} else {
			setpri(p);
		}
	}

	vmmeter();
	if (runin != 0) {
		runin = 0;
		wakeup((caddr_t)&runin);
	//	wakeup((caddr_t)pageproc);
	}
	++runrun;			/* swtch at least once a second */
	timeout(schedcpu, (caddr_t)0, hz);
}
