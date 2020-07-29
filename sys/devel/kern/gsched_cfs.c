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

/* alternatively make a sorted rb_tree */
/* p_estcpu equals cfs_decay upon being adding to the cfs run queue */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <sys/gsched_cfs.h>

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
/* Modified 4.4BSD-Lite2: cpu decay. Accounts for a process's priority weighting
 * Changes means that a process's priority weighting causes either of the following to occur:
 * 1. A higher priority weighting equals a lower cpu decay.
 * 2. A lower priority weighting equals a higher cpu decay.
 */

/* calculations for digital decay to forget 90% of usage in 5*loadav sec */
#define	loadfactor(loadav)		        (2 * (loadav))
#define priwfactor(loadav, priweight)   (loadfactor(loadav) / (priweight))
#define	decay_cpu(loadfac, cpu)	        (((loadfac) * (cpu)) / ((loadfac) + FSCALE))

/* make part of gsched, it potentially could be used elsewhere  */

/* cpu decay  */
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
    if(p->p_estcpu == 0) {
    	p->p_estcpu = p->p_cpu;
    }
    newcpu = (u_int) decay_cpu(loadfac, p->p_estcpu) + p->p_nice;
    return (newcpu);
}

/* make part of gsched, it potentially could be used elsewhere  */

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
		p->p_slptime--;	/* the first time was done in schedcpu */
		while (newcpu && --p->p_slptime)
			newcpu = (int) decay_cpu(loadfac, newcpu);
		p->p_estcpu = min(newcpu, UCHAR_MAX);
	}
	resetpri(p); /* potentially */
}

RB_PROTOTYPE(gsched_cfs_rbtree, gsched_cfs, cfs_entry, cfs_rb_compare);
RB_GENERATE(gsched_cfs_rbtree, gsched_cfs, cfs_entry, cfs_rb_compare);

/* merge with gsched_compare (do same thing) or alter to priority weighting */
int
cfs_rb_compare(a, b)
	struct gsched_cfs *a, *b;
{
	if(a->cfs_cpticks < b->cfs_cpticks) {
		return (-1);
	} else if (a->cfs_cpticks > b->cfs_cpticks) {
		return (1);
	}
	return (0);
}

/* insert into tree */
void
cfs_enqueue(cfs, p)
	struct gsched_cfs *cfs;
	struct proc *p;
{
	cfs->cfs_estcpu = cfs_decay(p, cfs->cfs_priweight);
	/* compare proc's estcpu & cfs estcpu */
	if(cfs->cfs_estcpu != p->p_estcpu) {
		if(cfs->cfs_estcpu > p->p_estcpu) {
			//Do something here:
		} else {
			//Do something here
		}
	}
	RB_INSERT(gsched_cfs_rbtree, &(cfs)->cfs_parent, p);
}

/* remove from tree */
void
cfs_dequeue(cfs, p)
	struct gsched_cfs *cfs;
	struct proc *p;
{
	RB_REMOVE(gsched_cfs_rbtree, &(cfs)->cfs_parent, p);
}

/* search for process with earliest deadline in rbtree & return */
cfs_search(cfs, p)
	struct gsched_cfs *cfs;
	struct proc *p;
{
	cfs->cfs_proc = RB_FIND(gsched_cfs_rbtree, cfs->cfs_parent, p);
	RB_FOREACH(p, gsched_cfs_rbtree, &(cfs)->cfs_parent) {
		if(cfs->cfs_cpticks == p->p_cpticks) {

		}
	}
}

void
cfs_compute(cfs)
	struct gsched_cfs *cfs;
{
	cfs->cfs_btl = BTL;
	cfs->cfs_bmg = BMG;
	cfs->cfs_btimeslice = BTIMESLICE(cfs->cfs_time);
	cfs->cfs_bsched = BRESCHEDULE;
}

/*
 * TODO:
 * Passed from EDF to CFS
 * Note: below assumes task is schedulable and inserted into rb tree
 * CFS:
 * 1. calculate cfs variables (timeslice, granularity, target latency)
 * 2. check granularity flags (aka. EMG)
 * 3. run through rbtree until reaching task with highest priority weighting.
 * 4. set task runnable with highest priority weighting.
 * 5. run task for timeslice period or base minimum time before a reschedule (BRESCHEDULE)
 * 6. check reschedule flags (aka. ERESCHEDULE) whether a reschedule or timeslice has expired
 *
 * 7. Two Scenarios:
 * A) Task is completed and exits run-queue.
 * B) Task is incomplete: exits rb_tree, and passed back to the EDF for re-processing
 *
 * Note: This doesn't include preemption or cpu_decay
 */

void
cfs_schedcpu(p)
	struct proc *p;
{
	struct gsched_cfs *cfs = gsched_cfs(p->p_gsched);
	u_char tmg = 0; 		/* temp min granularity */
	u_char tsched = 0; 		/* temp reschedule time */

	if(p->p_gsched->gsc_priweight != 0) {
		cfs->cfs_priweight = p->p_gsched->gsc_priweight;
		/* setrunq here? */
	}

	/* calculate cfs variables */
	cfs_compute(cfs);

	/* check min granularity */
	if(EMG(cfs->cfs_time)) {
		tmg = BMG;
		BMG = tmg;
		cfs->cfs_bmg = BMG; /* Not needed? in loop... taken from cfs_compute */
	}

	/* TODO: search rb_tree for task with deadline */

	/* check if it has time. Shouldn't be possible but check anyway */
	if(cfs->cfs_time != 127) { /* if placed within schedcpu, not required */
		/* run task for timeslice */
		for(int i = 0; i <= cfs->cfs_btimeslice; i++) {
			/* TODO: needs execution code here */
			cfs->cfs_time++;
			if(cfs->cfs_time == 127) {
				break;
			}
			if(i == cfs->cfs_bsched) {
				break;
			}
		}
	}
	/* missing code here */

	/* check reschedule */
	if(ERESCHEDULE(cfs->cfs_time)) {
		tsched = (cfs->cfs_time * cfs->cfs_bmg);
		BRESCHEDULE = tsched;
		cfs->cfs_bsched = BRESCHEDULE; /* Not needed? in loop... taken from cfs_compute */
	}

	/* task complete: remove of queue */
	remrq(p);

	/* TODO: task incomplete: remove from CFS & re-enter EDF to update */
}
