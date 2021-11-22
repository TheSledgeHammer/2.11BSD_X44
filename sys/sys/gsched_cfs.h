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
 * CFS Scheduler Differences:
 * A New Scheduling Period is calculated on the slack/laxity time, over the number of tasks.
 *
 * Reasons For Change:
 * - Linux uses the number of tasks to help calculate priority weightings and time-slices.
 * - BSD's existing scheduler can already calculate time-slices and priorities/ priority weightings.
 *
 * Effects:
 * 1. A greater slack/laxity time equals a longer scheduling period.
 * 2. A longer scheduling period reduces the slack/laxity time between each task
 */

#ifndef _SYS_GSCHED_CFS_H
#define _SYS_GSCHED_CFS_H

#include <sys/tree.h>
#include "../../sys/gsched.h"

/* Generic Base stats for CFS */
#define BTL             		20                      			/* base target latency */
#define BMG            	 		4                       			/* base minimum granularity  */
#define BSCHEDULE     			(BTL / BMG)            				/* base scheduling period */

/* Error Checking */
#define EBSCHEDULE(slack, p)   	((slack) > (p)->cfs_bsched)  		/* new scheduling period if slack/laxity time exceeds base scheduling period (BTL/BMG) */

struct gsched_cfs_rbtree;
RB_HEAD(gsched_cfs_rbtree, gsched_cfs);
struct gsched_cfs {
	struct gsched 				*cfs_gsched;						/* pointer to global scheduler */

	struct gsched_cfs_rbtree	cfs_parent;							/* rbtree cfs parent/root */
    RB_ENTRY(gsched_cfs) 		cfs_entry;							/* rbtree cfs entries */

	struct proc 				*cfs_rqlink;						/* pointer to linked list of running processes */
	struct proc 				*cfs_proc;

    int	    					cfs_flag;
    char    					cfs_stat;

    u_int  	 					cfs_estcpu;
    int							cfs_cpticks;

    u_char  					cfs_pri;
    u_char  					cfs_cpu;
    u_char  					cfs_time;
    char    					cfs_slptime;

    u_char 						cfs_btl;							/* base target latency */
    u_char 						cfs_bmg;							/* base minimum granularity */
    u_char 						cfs_bsched;							/* base scheduling period */

    u_char  					cfs_priweight;						/* priority weighting (calculated from various scheduling factors) */
};

unsigned int 	cfs_decay(struct proc *, u_char);
unsigned int 	cfs_update(struct proc *, u_char);
int				cfs_schedcpu(struct proc *);

#endif /* _SYS_GSCHED_CFS_H */

/*

left side = tasks with the gravest need for the processor (lowest virtual runtime) are stored toward the left side of the tree
right side = tasks with the least need of the processor (highest virtual runtimes) are stored toward the right side of the tree

Overview of CFS

The scheduler then, to be fair, picks the left-most node of the red-black tree to schedule next to maintain fairness.
The task accounts for its time with the CPU by adding its execution time to the virtual runtime and is then inserted
back into the tree if runnable. In this way, tasks on the left side of the tree are given time to execute, and the
contents of the tree migrate from the right to the left to maintain fairness. Therefore, each runnable task chases
the other to maintain a balance of execution across the set of runnable tasks.

Priorities and CFS

CFS doesn't use priorities directly but instead uses them as a decay factor for the time a task is permitted to execute.
Lower-priority tasks have higher factors of decay, where higher-priority tasks have lower factors of delay.
This means that the time a task is permitted to execute dissipates more quickly for a lower-priority task than for a
higher-priority task. That's an elegant solution to avoid maintaining run queues per priority.

Scheduling classes and domains

Also introduced with CFS is the idea of scheduling classes (recall from Figure 2). Each task belongs to a scheduling class,
which determines how a task will be scheduled. A scheduling class defines a common set of functions (via sched_class) that
define the behavior of the scheduler. For example, each scheduler provides a way to add a task to be scheduled, pull the
next task to be run, yield to the scheduler, and so on. Each scheduler class is linked with one another in a singly linked list,
allowing the classes to be iterated (for example, for the purposes of enablement of disablement on a given processor).
The general structure is shown in Figure 3. Note here that enqueue and dequeue task functions simply add or remove a task from
the particular scheduling structures. The function pick_next_task chooses the next task to execute (depending upon the particular
policy of the scheduling class).

Scheduling classes are yet another interesting aspect of the scheduling changes, but the functionality grows with the addition
of scheduling domains. These domains allow you to group one or more processors hierarchically for purposes load balancing and
segregation. One or more processors can share scheduling policies (and load balance between them) or implement independent
scheduling policies to intentionally segregate tasks.

*/
