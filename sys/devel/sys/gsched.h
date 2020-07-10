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

#ifndef _SYS_GSCHED_H
#define _SYS_GSCHED_H

#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/queue.h>

/* Schedulers + Info used by both EDF & CFS */
/* global scheduler */
struct gsched {
	struct proc 		*gsc_rqlink; 	/* pointer to linked list of running processes */
	struct proc 		*gsc_proc;		/* pointer to proc */

	struct lock			*gsc_lock;

    struct gsched_edf	*gsc_edf;		/* earliest deadline first scheduler */
    struct gsched_cfs 	*gsc_cfs;		/* completely fair scheduler */

    /* Proc Specific */
    u_int				gsc_estcpu;	 	/* Time averaged value of p_cpticks. */
    int					gsc_cpticks;	/* Ticks of cpu time. */

    u_char				gsc_pri;		/* Process  priority, negative is high */
    u_char				gsc_cpu;		/* cpu usage for scheduling */
    u_char				gsc_time;		/* resident time for scheduling */
    char 				gsc_slptime; 	/* sleep time in secs */

    /* Global Scheduler Specific */
    u_char  			gsc_priweight;	/* priority weighting (calculated from various factors) */

    char 				gsc_release;	/* Time till release from current block */
    char 				gsc_remtime; 	/* time remaining */

};

/* Scheduler Domains: Hyperthreading, multi-cpu, etc... */
struct sched_domains {

};

/* Priority Weighting Factors */
enum priw {
	 PW_PRIORITY = 25, 	 	/* Current Processes Priority */
	 PW_DEADLINE = 25,		/* Current Processes Deadline Time */
	 PW_RELEASE = 25,		/* Current Processes Release Time */
	 PW_SLEEP = 25,			/* Current Processes Sleep Time */
};

//#define PW_PRIORITY 25      /* Current Processes Priority */
//#define PW_DEADLINE 25      /* Current Processes Deadline Time */
//#define PW_RELEASE  25      /* Current Processes Release Time */
//#define PW_SLEEP    25      /* Current Processes Sleep Time */

#define PW_FACTOR(w, f)  ((float)(w) / 100 * (f)) /* w's weighting for a given factor(f)(above) */

void 				gsched_init( struct proc *);
void				gsched_edf_setup(struct gsched *);
void				gsched_cfs_setup(struct gsched *);
void 				gsched_setrq(struct gsched *);
void 				gsched_remrq(struct gsched *);
struct proc			*gsched_getrq(struct gsched *);
struct gsched_edf 	*gsched_edf(struct gsched *);
struct gsched_cfs 	*gsched_cfs(struct gsched *);
u_char				gsched_timediff(u_char, u_int);
int					gsched_setpriweight(float pwp, float pwd, float pwr, float pws);
void				gsched_lock(struct gsched *);
void				gsched_unlock(struct gsched *);

#endif /* _SYS_GSCHED_H */
/*
 priority weighting: replace release time with cpu utilization.

cost/usage = p_cpu

time = p_estcpu & p_time. (factor p_estcpu if task does not complete in given time)

deadline = p_cpticks

release = if deadline is greater than time. release is 0. else release = time
- if release is 0, schedulability should fail. (Impossible to complete a task by a deadline which is greater than the time given)

slack = (deadline - time) - cost

remtime = calculated at the end of CFS run queue
*/


/* Load average structure. */

/* calculations for digital decay to forget 90% of usage in 5*loadav sec */
//#define	loadfactor(loadav)	(2 * (loadav))
