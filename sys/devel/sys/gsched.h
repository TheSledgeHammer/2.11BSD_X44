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
 * Global Scheduler Design:
 * Preemption: (Not implemented)
 *  - Preempt: Determined by the priority weighting & the preemption flag
 *  	- 1. Insertion into CFS. If task to be inserted is considered a higher priority than the current running task
 * 		- 2. When the task is being recalculated (i.e. check deadline)
 * 		- 3. EDF cannot enforce a preempt, only suggest (via a preemption flag)
 * 			- Unless a process has been move to a different run-queue (2. is the only scenario this could happen)
 * 		- 4. CFS prefers not preempt.
 * 			- Will preempt if the priority weighting on a process is higher than the current process & the process
 * 			was flagged by EDF.
 * Hierarchy:
 * - 2.11BSD Scheduler: (aka kernel_synch.c)
 * 		- Same responsibilities as before
 * - EDF:
 * 		- Executes within schedcpu in kernel_synch.c
 * 		- Runs on a per run-queue basis (i.e. single run-queue)
 * 		- Determines the schedulability (i.e. schedulability test)
 * 		- Determines the priority weighting.
 * 		- Organizes run-queue
 * 		- Generates necessary information from kernel
 * 		- Passes onto CFS if process is schedulable
 * - CFS:
 * 		- Executes within schedcpu in kernel_synch.c
 * 		- Runs on multiple run-queues
 * 		- Only executes a process that passes the schedulability test
 * 		- Uses the information provided by EDF to determine:
 * 			- CPU Decay
 * 			- Run-time per process
 * 			- Process's preferable order
 *		- Passes back to the 2.11BSD Scheduler or exits
 *
 * For the implementation:
 * Please look at the following:
 *  - devel/sys/gsched_cfs.h
 *  - devel/sys/gsched_edf.h
 *  - devel/kern/sys_gsched_cfs.c
 *  - devel/kern/sys_gsched_edf.c
 *  - devel/kern/sys_gsched.c
 *  - devel/kern/sched.c
 *
 * Scheduler Decision Tree:
 * 																						 --> Process Complete --> Process Exits
 * 																						 |
 * 										  					--> Pass --> CFS Scheduler --|
 * 										  					|							 |
 * 										  					|							 --> Process Incomplete --> 2.11BSD Scheduler
 *  Process Enters -> 2.11BSD Scheduler --> EDF Scheduler --|
 *  									  					|
 *  									  					|
 *  									  					--> Fail --> 2.11BSD Scheduler
 *  Notes:
 *  Due to how 2.11BSD's (& 4.4BSD's) scheduler works, all process's will exit the run-queue
 *  (irrelevant of processor time needed) before re-entering.
 *  Which is not shown in the above decision tree for the sake of simplicity.
 */

#ifndef _SYS_GSCHED_H
#define _SYS_GSCHED_H

#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/queue.h>

/* Schedulers & Common Information used across schedulers */
/* global scheduler */
struct gsched {
	struct proc 			*gsc_rqlink; 	/* pointer to linked list of running processes */
	struct proc 			*gsc_proc;		/* pointer to proc */

    u_char  				gsc_priweight;	/* priority weighting: see below. */

    /* pointer to schedulers */
    struct gsched_edf		*gsc_edf;		/* earliest deadline first scheduler */
    struct gsched_cfs 		*gsc_cfs;		/* completely fair scheduler */
};

/* Linux Concept: Scheduler Domains: Hyperthreading, multi-cpu */
union gsched_group {
	CIRCLEQ_ENTRY(gsd_group) gsg_entry;
};

struct gsched_grphead;
CIRCLEQ_HEAD(gsched_grphead, gsched_group);
struct gsched_domain {
	/* Not Implemented */
	struct gsched_grphead	 *gsd_header;
	int 					 gsd_nentries;
	int 					 gsd_refcnt;
};

/* Priority Weighting Factors */
enum priw {
	 PW_PRIORITY = 25, 	 	/* Current Processes Priority */
	 PW_DEADLINE = 25,		/* Current Processes Deadline Time */
	 PW_SLEEP = 25,			/* Current Processes Sleep Time */
	 PW_LAXITY = 25,		/* Current Processes Laxity/Slack Time */
};

#define PW_FACTOR(w, f)  ((float)(w) / 100 * (f)) /* w's weighting for a given factor(f)(above) */

/*
 * Priority Weighting Calculation:
 * XXX: Needs Tuning: currently any factor below 4 results in 0 (with above weightings).
 */
#define PRIORITY_WEIGHTING(pw, pwp, pwd, pwl, pws) {		\
	(pw) = 	((PW_FACTOR((pwp), PW_PRIORITY) +  				\
			PW_FACTOR((pwd), PW_DEADLINE) + 				\
			PW_FACTOR((pwl), PW_LAXITY) + 					\
			PW_FACTOR((pws), PW_SLEEP)) / 4); 				\
};

void 				gsched_init(struct proc *);
void				gsched_edf_setup(struct gsched *, struct proc *);
void				gsched_cfs_setup(struct gsched *, struct proc *);
struct proc			*gsched_proc(struct gsched *);
struct gsched_edf 	*gsched_edf(struct gsched *);
struct gsched_cfs 	*gsched_cfs(struct gsched *);
u_char				gsched_timediff(u_char, u_int);
void				gsched_sort(struct proc *, struct proc *);

#endif /* _SYS_GSCHED_H */
