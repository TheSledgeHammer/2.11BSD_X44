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

