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
 * 		- Priorities, Run-Queues, etc
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
 *  - sys/sched_cfs.h
 *  - sys/sched_edf.h
 *  - kern/kern_sched.c
 *  - kern/kern_synch.c
 *  - kern/sched_cfs.c
 *  - kern/sched_edf.c
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

#ifndef _SYS_SCHED_H
#define _SYS_SCHED_H

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/queue.h>

/* Schedulers & Common Information used across schedulers */
/* global scheduler */
struct sched {
	struct proc 			*gsc_rqlink; 	/* pointer to linked list of running processes */
	struct proc 			*gsc_proc;		/* pointer to proc */

    u_char  				gsc_priweight;	/* priority weighting: see below. */
    /* priority weight factors */
    u_char					gsc_slack;		/* slack / laxity time */
    u_char					gsc_utilization;/* utilization per task */
    u_char					gsc_demand;		/* demand per task */
    u_char					gsc_workload;	/* workload per task */

    /* pointer to schedulers */
    struct sched_edf		*gsc_edf;		/* earliest deadline first scheduler */
    struct sched_cfs 		*gsc_cfs;		/* completely fair scheduler */
};

/* Scheduler Domains: Hyperthreading, multi-cpu */
/* Not Implemented */
struct sched_group {
	CIRCLEQ_ENTRY(sched_group) gsg_entry;
};

/* Not Implemented */
struct sched_grphead;
CIRCLEQ_HEAD(sched_grphead, sched_group);
struct sched_domain {
	struct sched_grphead	gsd_header;
	int 					gsd_nentries;
	int 					gsd_refcnt;
};


/* Priority Weighting Factors */
#define	PW_PRIORITY 		25 	 			/* Current Processes Priority */
#define	PW_DEADLINE 		25				/* Current Processes Deadline Time */
#define	PW_SLEEP 			25				/* Current Processes Sleep Time */
#define PW_LAXITY 			25				/* Current Processes Laxity/Slack Time */
#define PW_FACTOR(w, f)  	percent(w, f) 	/* w's weighting for a given factor(f) see above */

/*
 * Priority Weighting Calculation:
 */
#define PRIORITY_WEIGHTING(pwp, pwd, pwl, pws)				\
		((PW_FACTOR((pwp), PW_PRIORITY) +  					\
				PW_FACTOR((pwd), PW_DEADLINE) + 			\
				PW_FACTOR((pwl), PW_LAXITY) + 				\
				PW_FACTOR((pws), PW_SLEEP)) / (4));

#ifdef _KERNEL
void 				sched_init(struct proc *);
struct proc			*sched_proc(struct sched *);
struct sched_edf 	*sched_edf(struct sched *);
struct sched_cfs 	*sched_cfs(struct sched *);
void				sched_estcpu(u_int, u_int);
void				sched_cpticks(int, int);
int					sched_compare(const void *, const void *);
void				sched_sort(struct proc *);
#endif /* _KERNEL */
#endif /* _SYS_SCHED_H */
