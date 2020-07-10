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

RB_PROTOTYPE(gsched_cfs_rbtree, gsched_cfs, cfs_entry, cfs_rb_compare);
RB_GENERATE(gsched_cfs_rbtree, gsched_cfs, cfs_entry, cfs_rb_compare);


#define	cpu_decay_left(loadfac, cpu)	(((loadfac) * (cpu)) / ((loadfac) + FSCALE))
#define	cpu_decay_right(loadfac, cpu)	(((loadfac) * (cpu)) / ((loadfac) + FSCALE))

int
cfs_rb_compare(a, b)
	struct gsched_cfs *a, *b;
{
	if(a->cfs_priweight < b->cfs_priweight) {
		return (-1);
	} else if (a->cfs_priweight > b->cfs_priweight) {
		return (1);
	}
	return (0);
}

cfs_insert_rq(cfs)
	struct gsched_cfs *cfs;
{
	RB_LEFT(cfs, cfs_entry);
	RB_RIGHT(cfs, cfs_entry);
	RB_INSERT(gsched_cfs_rbtree, &(cfs)->cfs_parent, cfs->cfs_rqlink);
}

cfs_remove_rq(cfs)
	struct gsched_cfs *cfs;
{
	RB_REMOVE(gsched_cfs_rbtree, &(cfs)->cfs_parent, cfs->cfs_rqlink);
}

void
cfs_schedcpu(gsd)
	struct gsched *gsd;
{
	struct gsched_cfs *cfs = gsched_cfs(gsd);

	if(gsd->gsc_priweight != 0) {
		cfs->cfs_priweight = gsd->gsc_priweight;
	}
	cfs->cfs_tl = BTL;
	cfs->cfs_mg = BMG;
	cfs->cfs_timeslice = BTIMESLICE(cfs->cfs_time);

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
	 * Two Scenarios:
	 * A) Task is completed and exits run-queue.
	 * B) Task is incomplete: exits rb_tree, and passed back to the EDF for re-processing
	 *
	 * Note: This doesn't include Preemption
	 */
}

u_char
cfs_base_timeslice(time)
	u_char time;
{
	return (BTIMESLICE(time));
}

u_char
cfs_reschedule(time)
	u_char time;
{
	return (ERESCHEDULE(time));
}

u_char
cfs_min_granularity(time)
	u_char time;
{
	return (EMG(time));
}
