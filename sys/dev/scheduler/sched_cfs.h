//
// Created by marti on 8/02/2020.
//

#ifndef SIMPLE_LIST_OF_POOLS_SCHED_CFS_H
#define SIMPLE_LIST_OF_POOLS_SCHED_CFS_H

#include <sys/proc.h>
#include <sys/tree.h>

/* Generic Base stats for CFS */
#define NCFSQS 	        8 					    /* 8 CFS Queues */
#define CFQS 	        (NQS/NCFSQS)		    /* Number of CFS Queues to Number of Run Queues (32/8 = 4) */
#define BTL             20                      /* base target latency */
#define BMG             4                       /* base minimum granularity  */
#define BTIMESLICE(t)   ((t) / BTL)             /* base timeslice per task */
#define BRESCHEDULE     (BTL / BMG)             /* base minimum time for (n * tasks) before rescheduling occurs */

/* Error Checking */
#define ERESCHEDULE(t)  ((t) * BMG)             /* new rescheduling time if (n * tasks) exceeds BTL/BMG */
#define EMG(t)          (BMG > BTIMESLICE(t))   /* new minimum granularity if BMG is greater than base timeslice */

struct sched_cfs {

    struct sched 	 *cfs_sched;	/* pointer to proc sched */
    struct sched_edf *cfs_edf; 		/* pointer to EDF */

    RB_HEAD(rb_cfs, sched_cfs) 	cfs_parent;
    RB_ENTRY(sched_cfs) 		cfs_queue;

    int	    cfs_flag;
    char    cfs_stat;

    u_int   cfs_estcpu;
    int		cfs_cpticks;
    fixpt_t cfs_pctcpu;

    u_char  cfs_pri;
    u_char  cfs_cpu;
    u_char  cfs_time;
    char    cfs_nice;
    char    cfs_slptime;

    u_char 	cfs_priweight;				/* priority weighting */

    /* CFS Relavent Stats */
    u_char 	cfs_tl;
    u_char 	cfs_mg;
    u_char 	cfs_timeslice;

    /* EDF Relavent Stats */
    char 	cfs_release;				/* Time till release from current block */
    char 	cfs_deadline;				/* Deadline */
    char 	cfs_remtime; 				/* time remaining */
};

extern struct sched_cfs cfs_runq[CFQS];  /* cfs run-queues */

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

CFS doesn’t use priorities directly but instead uses them as a decay factor for the time a task is permitted to execute.
Lower-priority tasks have higher factors of decay, where higher-priority tasks have lower factors of delay.
This means that the time a task is permitted to execute dissipates more quickly for a lower-priority task than for a
higher-priority task. That’s an elegant solution to avoid maintaining run queues per priority.

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

next
Enqueue
Dequeue
yield task
check preempt
pick next tsk
put prev task
*/
#endif //SIMPLE_LIST_OF_POOLS_SCHED_CFS_H
