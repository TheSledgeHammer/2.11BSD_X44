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
#define	loadfactor(loadav)		        (2 * (loadav))
#define	decay_cpu(loadfac, cpu)	        (((loadfac) * (cpu)) / ((loadfac) + FSCALE))

/* cpu decay  */
unsigned int
cpu_decay(p)
	register struct proc *p;
{
	register fixpt_t loadfac;
	register unsigned int newcpu;

	loadfac = loadfactor(averunnable.ldavg[0]);
    if (p->p_estcpu == 0) {
    	p->p_estcpu = p->p_cpu;
    }
    newcpu = (u_int)decay_cpu(loadfac, p->p_estcpu) + p->p_nice;
    return (newcpu);
}

/* update cpu decay */
void
cpu_update(p)
	register struct proc *p;
{
	register unsigned int newcpu;
	register fixpt_t loadfac;

	newcpu = p->p_estcpu;
	loadfac = loadfactor(averunnable.ldavg[0]);
	if (p->p_slptime > 5 * loadfac) {
		p->p_estcpu = 0;
	} else {
		p->p_slptime--; /* 2nd round: (1st round in schedcpu) */
		while (newcpu && --p->p_slptime) {
			newcpu = (u_int)decay_cpu(loadfac, newcpu);
		}
		p->p_estcpu = min(newcpu, UCHAR_MAX);
	}
}

unsigned int
cfs_decay(p, priweight)
	register struct proc *p;
	u_char priweight;
{
	return (cpu_decay(p));
}

void
cfs_update(p, priweight)
	register struct proc *p;
	u_char priweight;
{
	cpu_update(p);
}

/*
 * ISSUE:
 * - priority weighting breaks above cpu decay algorithm.
 *
 * TODO:
 * - change the priwfactor from being directly calculated of the
 * 		loadfactor in cfs_decay & cfs_update.
 */
/*
 * Modified 4.4BSD-Lite2 CPU Decay Calculation:
 * - To account for a process's priority weighting.
 * - The priority weighting causes either of the following to occur:
 * 		1. Higher priority weighting equals a lower CPU decay.
 * 		2. Lower priority weighting equals a higher CPU decay.
 */
#ifdef notyet
#define priwfactor(loadav, priweight)   (loadfactor(loadav) / (priweight))

unsigned int
cfs_decay(p, priweight)
	register struct proc *p;
	u_char priweight;
{
	register fixpt_t loadfac;
	register unsigned int newcpu;

    if (priweight == 0) {
        loadfac = loadfactor(averunnable.ldavg[0]);
    } else {
        loadfac = priwfactor(averunnable.ldavg[0], priweight);
    }
    if (p->p_estcpu == 0) {
    	p->p_estcpu = p->p_cpu;
    }
    newcpu = (u_int)decay_cpu(loadfac, p->p_estcpu) + p->p_nice;
    return (newcpu);
}

/* update cpu decay */
void
cfs_update(p, priweight)
	register struct proc *p;
	u_char priweight;
{
	register unsigned int newcpu = p->p_estcpu;
	register fixpt_t loadfac;

    if (priweight == 0) {
        loadfac = loadfactor(averunnable.ldavg[0]);
    } else {
        loadfac = priwfactor(averunnable.ldavg[0], priweight);
    }

	if (p->p_slptime > 5 * loadfac) {
		p->p_estcpu = 0;
	} else {
		p->p_slptime--;						/* 2nd round: (1st round in schedcpu) */
		while (newcpu && --p->p_slptime) {
			newcpu = (int)decay_cpu(loadfac, newcpu);
		}
		p->p_estcpu = min(newcpu, UCHAR_MAX);
	}
}
#endif

int
cfs_rb_compare(a, b)
	struct gsched_cfs *a, *b;
{
	if (a->cfs_cpticks < b->cfs_cpticks) {
		return (-1);
	} else if (a->cfs_cpticks > b->cfs_cpticks) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(gsched_cfs_rbtree, gsched_cfs, cfs_entry, cfs_rb_compare);
RB_GENERATE(gsched_cfs_rbtree, gsched_cfs, cfs_entry, cfs_rb_compare);

/*
 * calculate base scheduling period
 * calculated from slack/ laxity time.
 */
void
cfs_compute(cfs, slack)
	struct gsched_cfs *cfs;
	u_char slack;
{
	u_char new_bsched; 			/* new base schedule period */

	if (EBSCHEDULE(slack, cfs)) {
		new_bsched = (slack * cfs->cfs_bmg);
		cfs->cfs_bsched = new_bsched;
	}
}

int
cfs_schedcpu(p)
	struct proc *p;
{
	register struct gsched_cfs *cfs = gsched_cfs(p->p_gsched);

	int cpticks = 0;			/* p_cpticks counter (deadline) */
	u_char slack;				/* slack/laxity time */

	slack = p->p_gsched->gsc_slack;
	if (p->p_gsched->gsc_priweight != 0) {
		cfs->cfs_priweight = p->p_gsched->gsc_priweight;
	}

	/* add to cfs queue */
	cfs->cfs_estcpu = cfs_decay(p, cfs->cfs_priweight);
	gsched_estcpu(cfs->cfs_estcpu, p->p_estcpu);
	RB_INSERT(gsched_cfs_rbtree, &cfs->cfs_parent, cfs);
	/* add to run-queue */
	setrq(p);

	/* calculate cfs variables */
	cfs_compute(cfs, slack);

	/* run-through red-black tree */
	for (p = RB_FIRST(gsched_cfs_rbtree, &cfs->cfs_parent)->cfs_proc; p != NULL; p = RB_LEFT(cfs, cfs_entry)->cfs_proc) {
		/* set cpticks */
		gsched_cpticks(cfs->cfs_cpticks, p->p_cpticks);
		/* start deadline counter */
		while (cpticks < cfs->cfs_cpticks) {
			cpticks++;
			/* Test if deadline was reached before end of scheduling period */
			if (cfs->cfs_cpticks == cpticks) {
				/* test if time doesn't equal the base scheduling period */
				if (cfs->cfs_time != cfs->cfs_bsched) {
					goto fin;
					break;
				} else {
					goto out;
					break;
				}
			} else if (cfs->cfs_cpticks != cpticks) {
				/* test if time equals the base scheduling period */
				if (cfs->cfs_time == cfs->cfs_bsched) {
					goto fin;
					break;
				} else {
					goto out;
					break;
				}
			} else {
				/* SHOULD NEVER REACH THIS POINT!! */
				/* panic?? */
				goto out;
				break;
			}
		}
	}

out:
	/* update cfs variables */
	cfs_update(p, cfs->cfs_priweight);
	/* remove from cfs queue */
	RB_REMOVE(gsched_cfs_rbtree, &cfs->cfs_parent, cfs);
	/* remove from run-queue */
	remrq(p);
	/* deadline reached before completion, reschedule */
	reschedule(p);

	return (0);

fin:
	/* update cfs variables */
	cfs_update(p, cfs->cfs_priweight);
	/* remove from cfs queue */
	RB_REMOVE(gsched_cfs_rbtree, &cfs->cfs_parent, cfs);
	/* remove from run-queue */
	remrq(p);

	return(0);
}
