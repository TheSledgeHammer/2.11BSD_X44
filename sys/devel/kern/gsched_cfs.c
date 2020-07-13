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

#include <sys/proc.h>
#include <sys/user.h>

#include "sys/gsched_cfs.h"

/* try: an array, to index deadline (example: cpt) */
/* alternatively make a sorted rb_tree */

/* cpu decay: A modified 4.4BSD decay that also accounts for a process's priority weighting */
#define	loadfactor(loadav)		        (2 * (loadav))
#define priwfactor(loadav, priweight)    (loadfactor(loadav) / (priweight))
#define	decay_cpu(loadfac, cpu)	        (((loadfac) * (cpu)) / ((loadfac) + FSCALE))

/* cpu decay  */
unsigned int
cfs_decay(p, priweight)
	register struct proc *p;
    int priweight;
{
	register fixpt_t loadfac;
	register unsigned int newcpu;

    if (priweight == 0) {
        loadfac = loadfactor(averunnable.ldavg[0]);
    } else {
        loadfac = priwfactor(averunnable.ldavg[0], priweight);
    }
    newcpu = (u_int) decay_cpu(loadfac, p->p_estcpu) + p->p_nice;
    return (newcpu);
}

RB_PROTOTYPE(gsched_cfs_rbtree, gsched_cfs, cfs_entry, cfs_rb_compare);
RB_GENERATE(gsched_cfs_rbtree, gsched_cfs, cfs_entry, cfs_rb_compare);

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
	/* compare proc's estcpu & cfs estcpu */
	cfs->cfs_estcpu = cfs_decay(p, cfs->cfs_priweight);
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
cfs_schedcpu(gsd)
	struct gsched *gsd;
{
	struct gsched_cfs *cfs = gsched_cfs(gsd);
	u_char tmg = 0; 		/* temp min granularity */
	u_char tsched = 0; 		/* temp reschedule time */

	if(gsd->gsc_priweight != 0) {
		cfs->cfs_priweight = gsd->gsc_priweight;
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

	/* TODO: search rb_tree for task with highest priority weighting */

	/* set task runnable */
	//setrun(cfs->cfs_proc);

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
	gsched_remrq(gsd);

	/* TODO: task incomplete: remove from CFS & re-enter EDF to update */
}
