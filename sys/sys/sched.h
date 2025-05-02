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

struct sched_factor;

/* Schedulers & Common Information used across schedulers */
/* global scheduler */
struct sched {
	struct proc 		*sc_procp;			/* pointer to proc */
	struct thread		*sc_threado;			/* pointer to thread */

	struct sched_factor 	*sc_factor;			/* schedule factor */

    union {
    	struct sched_edf	*u_edf;				/* earliest deadline first scheduler */
    	struct sched_cfs 	*u_cfs;				/* completely fair scheduler */
    } sc_u;
#define sc_edf			sc_u.u_edf
#define sc_cfs			sc_u.u_cfs

    /* schedule base factors */
    u_char  			sc_priweight;			/* priority weighting: see below. */
    u_char			sc_slack;			/* slack / laxity time */
    u_char			sc_utilization;			/* utilization */
    u_char			sc_demand;			/* demand */
    u_char			sc_workload;			/* workload */
};

/* schedule factors */
struct sched_factor {
    union {
        u_char  factor;						/* utilization factor */
        int 	rate;						/* utilization rate */
        int	weight; 					/* utilization weight */
    } sf_utilization;
#define sfu_utilization    	sf_utilization.factor
#define sfu_rate           	sf_utilization.rate
#define sfu_weight         	sf_utilization.weight
    union {
        u_char  factor;						/* demand factor */
        int 	rate;						/* demand rate */
        int	weight;						/* demand weight */
    } sf_demand;
#define sfd_demand         	sf_demand.factor
#define sfd_rate           	sf_demand.rate
#define sfd_weight         	sf_demand.weight
    union {
        u_char  factor;						/* workload factor */
        int 	rate;						/* workload rate */
        int	weight;						/* workload weight */
    } sf_workload;
#define sfw_workload       	sf_workload.factor
#define sfw_rate           	sf_workload.rate
#define sfw_weight         	sf_workload.weight

    int 			sf_avgrate;			/* average rate */
    int 			sf_avgweight;			/* average weight */
    int 			sf_optnthreads;			/* optimal number of threads */
};

/* Priority Weighting Factors */
#define	PW_PRIORITY 		25 	 			/* Current Processes Priority */
#define	PW_DEADLINE 		25				/* Current Processes Deadline Time */
#define	PW_SLEEP 		25				/* Current Processes Sleep Time */
#define PW_LAXITY 		25				/* Current Processes Laxity/Slack Time */
#define PW_FACTOR(w, f)  	percent(w, f) 			/* w's weighting for a given factor(f) see above */

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

/* Schedule Factors */

/* Schedule Rating Range */
static __inline int
sched_rate_range(val, min, max)
	int val, min, max;
{
	if (val < 0) {
		val = 0;
	}
	if (val > 100) {
		val = 100;
	}
	if ((val >= min) && (val <= max)) {
		return (0);
	}
	return (-1);
}

/* Schedule Rating */
#define SCHED_RATE_LOW(val)        	(sched_rate_range(val, 0, 33) == 0)
#define SCHED_RATE_MEDIUM(val)   	(sched_rate_range(val, 34, 66) == 0)
#define SCHED_RATE_HIGH(val)       	(sched_rate_range(val, 67, 100) == 0)

static __inline int
sched_rating(val)
	u_char val;
{
    if (SCHED_RATE_LOW(val)) {
    	return (val);
    }
    if (SCHED_RATE_MEDIUM(val)) {
    	return (val);
    }
    if (SCHED_RATE_HIGH(val)) {
    	return (val);
    }
    return (-1);
}

/* Schedule Weighting */
#define SCHED_WEIGHT_HIGH			9
#define SCHED_WEIGHT_MEDIUM			6
#define SCHED_WEIGHT_LOW 			3

static __inline int
sched_weighting(val)
	u_char val;
{
    if (SCHED_RATE_LOW(val)) {
        return (SCHED_WEIGHT_HIGH);
    }
    if (SCHED_RATE_MEDIUM(val)) {
        return (SCHED_WEIGHT_MEDIUM);
    }
    if (SCHED_RATE_HIGH(val)) {
        return (SCHED_WEIGHT_LOW);
    }
    return (-1);
}

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
void 	sched_init(struct proc *);
void	sched_estcpu(u_int, u_int);
void	sched_cpticks(int, int);
void	sched_compute(struct sched *, struct proc *);
void	sched_check_threads(struct sched *, struct proc *);
int	sched_create_threads(struct proc **, struct thread **, char *, size_t);
void	sched_destroy_threads(struct proc *, int, int);
#endif /* _KERNEL */
#endif /* _SYS_SCHED_H */
