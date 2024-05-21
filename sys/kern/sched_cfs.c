/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Global Scheduler's: Completely Fair Scheduling Algorithm.
 * Designed to schedule on multiple run-queues.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/sched.h>
#include <sys/sched_cfs.h>
#include <sys/sched_edf.h>

#include <vm/include/vm.h>

#include <machine/cpu.h>

/*
 * Constants for digital decay and forget:
 *	90% of (p_estcpu) usage in 5 * loadav time
 *	95% of (p_pctcpu) usage in 60 seconds (load insensitive)
 *          Note that, as ps(1) mentions, this can let percentages
 *          total over 100% (I've seen 137.9% for 3 processes).
 *
 * Note that hardclock updates p_estcpu and p_cpticks independently.
 *
 * We wish to decay away 90% of p_estcpu in (5 * loadavg) seconds.
 * That is, the system wants to compute a value of decay such
 * that the following for loop:
 * 	for (i = 0; i < (5 * loadavg); i++)
 * 		p_estcpu *= decay;
 * will compute
 * 	p_estcpu *= 0.1;
 * for all values of loadavg:
 *
 * Mathematically this loop can be expressed by saying:
 * 	decay ** (5 * loadavg) ~= .1
 *
 * The system computes decay as:
 * 	decay = (2 * loadavg) / (2 * loadavg + 1)
 *
 * We wish to prove that the system's computation of decay
 * will always fulfill the equation:
 * 	decay ** (5 * loadavg) ~= .1
 *
 * If we compute b as:
 * 	b = 2 * loadavg
 * then
 * 	decay = b / (b + 1)
 *
 * We now need to prove two things:
 *	1) Given factor ** (5 * loadavg) ~= .1, prove factor == b/(b+1)
 *	2) Given b/(b+1) ** power ~= .1, prove power == (5 * loadavg)
 *
 * Facts:
 *         For x close to zero, exp(x) =~ 1 + x, since
 *              exp(x) = 0! + x**1/1! + x**2/2! + ... .
 *              therefore exp(-1/b) =~ 1 - (1/b) = (b-1)/b.
 *         For x close to zero, ln(1+x) =~ x, since
 *              ln(1+x) = x - x**2/2 + x**3/3 - ...     -1 < x < 1
 *              therefore ln(b/(b+1)) = ln(1 - 1/(b+1)) =~ -1/(b+1).
 *         ln(.1) =~ -2.30
 *
 * Proof of (1):
 *    Solve (factor)**(power) =~ .1 given power (5*loadav):
 *	solving for factor,
 *      ln(factor) =~ (-2.30/5*loadav), or
 *      factor =~ exp(-1/((5/2.30)*loadav)) =~ exp(-1/(2*loadav)) =
 *          exp(-1/b) =~ (b-1)/b =~ b/(b+1).                    QED
 *
 * Proof of (2):
 *    Solve (factor)**(power) =~ .1 given factor == (b/(b+1)):
 *	solving for power,
 *      power*ln(b/(b+1)) =~ -2.30, or
 *      power =~ 2.3 * (b + 1) = 4.6*loadav + 2.3 =~ 5*loadav.  QED
 *
 * Actual power values for the implemented algorithm are as follows:
 *      loadav: 1       2       3       4
 *      power:  5.68    10.32   14.94   19.55
 */
/* calculations for digital decay to forget 90% of usage in 5*loadav sec */
/*
 * Modified 4.4BSD-Lite2 CPU Decay Calculation:
 * - To account for a process's priority weighting.
 * - The priority weighting causes either of the following to occur:
 * 		1. Higher priority weighting equals a lower CPU decay.
 * 		2. Lower priority weighting equals a higher CPU decay.
 */
#define	loadfactor(loadav)				(2 * (loadav))
#define priwfactor(loadav, priweight)   (((2 * (loadav)) / (priweight)) * (priweight))
#define	decay_cpu(loadfac, cpu)	        (((loadfac) * (cpu)) / ((loadfac) + FSCALE))

/* cpu loadfactor */
fixpt_t
cfs_loadfactor(priweight)
 	 u_char priweight;
{
	 register fixpt_t loadfac, priwfac;

	 loadfac = loadfactor(averunnable.ldavg[0]);
	 priwfac = priwfactor(averunnable.ldavg[0], priweight);

	 if (loadfac > priwfac && priweight != 0) {
		 return (priwfac);
	 }
	 return (loadfac);
}

/* cpu decay  */
unsigned int
cfs_decay(p, priweight)
	register struct proc *p;
	u_char priweight;
{
	register fixpt_t load;
	register unsigned int newcpu;

	load = cfs_loadfactor(priweight);
	if (p->p_estcpu == 0) {
		p->p_estcpu = p->p_cpu;
	}
	newcpu = (u_int)decay_cpu(load, p->p_estcpu) + p->p_nice;
	return (newcpu);
}

/* update cpu decay */
void
cfs_update(p, priweight)
	register struct proc *p;
	u_char priweight;
{
	register fixpt_t load;
	register unsigned int newcpu;

	newcpu = p->p_estcpu;
	load = cfs_loadfactor(priweight);
	if (p->p_slptime > 5 * load) {
		p->p_estcpu = 0;
	} else {
		p->p_slptime--; /* 2nd round: (1st round in schedcpu) */
		while (newcpu && --p->p_slptime) {
			newcpu = (u_int)decay_cpu(load, newcpu);
		}
		p->p_estcpu = min(newcpu, UCHAR_MAX);
	}
}

int
cfs_rb_compare(a, b)
	struct sched_cfs *a, *b;
{
	if (a->cfs_cpticks < b->cfs_cpticks) {
		return (-1);
	} else if (a->cfs_cpticks > b->cfs_cpticks) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(sched_cfs_rbtree, sched_cfs, cfs_entry, cfs_rb_compare);
RB_GENERATE(sched_cfs_rbtree, sched_cfs, cfs_entry, cfs_rb_compare);

void
cfs_compute(cfs, p)
	struct sched_cfs *cfs;
	struct proc *p;
{
	cfs->cfs_cpticks = p->p_cpticks;
	cfs->cfs_pri = p->p_pri;
	cfs->cfs_cpu = p->p_cpu;
	cfs->cfs_time = p->p_time;
	cfs->cfs_slptime = p->p_slptime;
}

int
cfs_schedcpu(p)
	struct proc *p;
{
	register struct sched *sc;
	register struct sched_cfs *cfs;
	struct sched_cfs *cfsp;		/* cfs process */
	int cpticks;				/* p_cpticks counter (deadline) */
	u_char new_bsched; 			/* new base schedule period */

	cpticks = 0;

	sc = p->p_sched;
	cfs_compute(cfs, p);
	cfs->cfs_slack = sc->sc_slack;
	cfs->cfs_priweight = sc->sc_priweight;
	cfs->cfs_estcpu = cfs_decay(p, cfs->cfs_priweight);
	sched_estcpu(cfs->cfs_estcpu, p->p_estcpu);

	/* add to cfs queue */
	RB_INSERT(sched_cfs_rbtree, &cfs->cfs_parent, cfs);

	/* calculate cfs base scheduling period */
	if (EBSCHEDULE(cfs->cfs_slack, cfs)) {
		new_bsched = (cfs->cfs_slack * cfs->cfs_bmg);
		cfs->cfs_bsched = new_bsched;
	}

	/* run-through red-black tree */
	for (cfs = RB_FIRST(sched_cfs_rbtree, &cfs->cfs_parent); cfs != NULL; cfs = RB_LEFT(cfs, cfs_entry)) {
		/* set cpticks */
		sched_cpticks(cfs->cfs_cpticks, p->p_cpticks);
		/* start deadline counter */
		while (cpticks < cfs->cfs_cpticks) {
			cpticks++;
			/* Test if deadline was reached before end of scheduling period */
			if (cfs->cfs_cpticks == cpticks) {
				/* test if time doesn't equal the base scheduling period */
				if (cfs->cfs_time != cfs->cfs_bsched) {
					cfsp = cfs;
					goto fin;
					break;
				} else {
					cfsp = cfs;
					goto out;
					break;
				}
			} else if (cfs->cfs_cpticks != cpticks) {
				/* test if time equals the base scheduling period */
				if (cfs->cfs_time == cfs->cfs_bsched) {
					cfsp = cfs;
					goto fin;
					break;
				} else {
					cfsp = cfs;
					goto out;
					break;
				}
			} else {
				/* SHOULD NEVER REACH THIS POINT!! */
				/* panic?? */
				cfsp = cfs;
				goto out;
				break;
			}
		}
	}

out:
	/* update cfs variables */
	cfs = cfsp;
	cfs_update(p, cfs->cfs_priweight);
	/* remove from cfs queue */
	RB_REMOVE(sched_cfs_rbtree, &cfs->cfs_parent, cfs);

	return (0);

fin:
	/* update cfs variables */
	cfs = cfsp;
	cfs_update(p, cfs->cfs_priweight);
	/* remove from cfs queue */
	RB_REMOVE(sched_cfs_rbtree, &cfs->cfs_parent, cfs);

	return (1);
}
