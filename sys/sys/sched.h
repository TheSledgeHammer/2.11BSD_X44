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
	struct proc 			*sc_procp;		/* pointer to proc */
	struct thread			*sc_threado;	/* pointer to thread */

	/* schedulers */
    union {
    	struct sched_edf	*u_edf;			/* earliest deadline first scheduler */
    	struct sched_cfs 	*u_cfs;			/* completely fair scheduler */
    } sc_u;
#define sc_edf				sc_u.u_edf
#define sc_cfs				sc_u.u_cfs

	/* scheduling proc / thread variables*/
    u_char  				sc_priweight;	/* priority weighting: see below. */
    u_char					sc_slack;		/* slack / laxity time */

    /* schedulability factors */
    u_char					sc_utilization;	/* utilization per task */
    u_char					sc_demand;		/* demand per task */
    u_char					sc_workload;	/* workload per task */

    /* schedulability rate */
	int						sc_utilrate;	/* utilization rate */
	int						sc_demandrate;	/* demand rate */
	int						sc_workrate;	/* workload rate */
	int						sc_avgrate;		/* average rate (determined from the above rates) */

    /* schedulability weight */
	int						sc_utilweight;	/* utilization weight */
	int						sc_demandweight;/* demand weight */
	int						sc_workweight;	/* workload weight */
	int						sc_avgweight;	/* average weight (determined from the above weights) */

	int 					sc_opt_nthreads;/* optimal number of threads for process */
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

/* Priority Weighting & Scheduling Macros */
#define SLACK(d, t, c)          (((d) - (t)) - (c))
#define UTILIZATION(c, r)       ((c) / (r))
#define DEMAND(t, d, r, c)      ((((t) - (d) + (r)) * (c)) / (r))
#define WORKLOAD(t, r, c)       (((t) / (r)) * (c))

/* Schedule Rating Range */
static __inline int
sched_rate_range(val, min, max)
	int val, min, max;
{
    if ((val >= min) && (val <= max)) {
        return (0);
    }
	return (1);
}

/* Schedule Rating */
#define SCHED_RATE_LOW(val)        	((sched_rate_range(val, 0, 33) == 0) || ((val) < 0))
#define SCHED_RATE_MEDIUM(val)   	(sched_rate_range(val, 34, 66) == 0)
#define SCHED_RATE_HIGH(val)       	((sched_rate_range(val, 67, 100) == 0) || ((val) > 100))

/* Schedule Weighting */
#define SCHED_WEIGHT_LOW			9
#define SCHED_WEIGHT_MEDIUM			6
#define SCHED_WEIGHT_HIGH 			3

#ifdef notyet
/* Scheduler Domains: Hyperthreading, multi-cpu */
/* Not Implemented */
struct sched_group {
	CIRCLEQ_ENTRY(sched_group) sg_entry;
};

/* Not Implemented */
struct sched_grphead;
CIRCLEQ_HEAD(sched_grphead, sched_group);
struct sched_domain {
	struct sched_grphead	sd_header;
	int 					sd_nentries;
	int 					sd_refcnt;
};
#endif

#ifdef _KERNEL
void 				sched_init(struct proc *);
struct sched_edf 	*sched_edf(struct sched *);
struct sched_cfs 	*sched_cfs(struct sched *);
void				sched_estcpu(u_int, u_int);
void				sched_cpticks(int, int);
void				sched_compute(struct sched *, struct proc *);
#endif /* _KERNEL */
#endif /* _SYS_SCHED_H */
