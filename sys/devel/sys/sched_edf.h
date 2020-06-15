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

#ifndef _SYS_SCHED_EDF_H
#define _SYS_SCHED_EDF_H

#include <sys/proc.h>
#include <sched.h>

struct sched_edf {
//    TAILQ_HEAD(edf, sched_edf) edf_parent;
	TAILQ_ENTRY(sched) 	edf_sc_entry;

	struct proc			*edf_proc;
 //   struct sched 		*edf_gsched;		/* pointer to proc sched */
 //   struct sched_cfs 	*edf_cfs; 		/* pointer to CFS */

    int	    			edf_flag;		/* P_* flags. */
    char    			edf_stat;		/* S* process status. */

    u_int   			edf_estcpu;		/* Time averaged value of p_cpticks. */
    int					edf_cpticks;	/* Ticks of cpu time. */
    fixpt_t 			edf_pctcpu;		/* %cpu for this process during p_swtime */

    u_int				edf_swtime;	 	/* Time swapped in or out. */

    u_char  			edf_pri;		/* Process  priority, negative is high */
    u_char  			edf_cpu;		/* cpu usage for scheduling */
    u_char  			edf_time;		/* resident time for scheduling */
    char   				edf_nice;		/* nice for cpu usage */
    char    			edf_slptime;	/* Time since last blocked. secs sleeping */

    char				edf_release;	/* Time till release from current block */
    char 				edf_deadline; 	/* Deadline */
    char 				edf_delta; 		/* Inherited Deadline */
    u_char 				edf_remtime; 	/* time remaining */

    u_char 				edf_priweight;	/* priority weighting */

    /* EFS's Schedulability Testing */
    char 				edf_testdelta;
    char 				edf_testrelease;
    char 				edf_testdeadline;

    struct proc 		*edf_testnext;
};

/*
Preemption:
    - Preempt Weighting: Calculated from Deadline, Priority and Preemption Flag
    - 1. Insertion into CFS. If task to be inserted is considered a higher priority than the current running task
    - 2. When the task is being recalculated (i.e. check deadline)

    Hierarchy:
        - EDF:
            - Cannot enforce a preempt, only suggested (preemption flag)
            - Unless task is being moved to a different rq (2. is the only scenario this could happen)
        - CFS:
            - Prefers not too.
            - If weighting on task is higher and the task has been flagged.

Check Deadline:
- Keep CFS & EDF in sync.
- After task has run at least once.
- This is where an interrupt (preemption) could take place.

Time-Slice per Run Queue:
- Determined by Earliest Deadline First Algorithm
- Tasks with a higher deadline (shorter time to completion) will be placed in a run queue with a shorter time-slice
- Task with a lower deadline (longer time to completion) will be placed in a run queue with a longer time-slice
- EDF determines Run Queue by which queue's timeslice is closest to the task's deadline.
    - A task with a lower deadline than a given rq timeslice is placed in the next lower rq (aka longer time-slice)
*/

/* Basic Concept */
/*
- Generalised routines as per kern_synch
- Scheduler expands on those rountines, in a pluggable manner
- All current rountines in kern_synch are generic for proc
- edf goals: maintain and prioritise the run queues, pass of to cfs
- cfs goals: Equal time, must adhere to edf's priorities. But providing fair time.
    Incomplete jobs are passed back to edf after x time or completed CFS loop
Multiple CFS?
 - Each priority queue runs its own CFS
    - Different sized priority queues?
    &/ OR
    - Different Time Slices (Could be calculated from EDF)?

 Start-up:
 - edf must start & have process in run-queue before cfs can start (rqinit)
*/

/* To Add: (multi-tasking/ threading) */
//preempt();
//dequeue();
//enqueue();
//lock();
//unlock();
//Add ways to see the wait queue?
//And Any specific needs of the scheduler

/* EDF Specific */
//testschedulability(); /* Test if process is able to be scheduled */

/* Scheduler Initilization */
//cfs_init();     /* start completely fair scheduler */
//edf_init();     /* start earliest deadline first scheduler */

//schedcpu();     /* starts scheduler calculations */
//rqinit();       /* starts doubly-linked run-queues */

/* Run-Queue */
//setrq();        /* Set run-queue */
//remrq();        /* Remove from run-queue */
//getrq();        /* Get run-queue */
//unsleep();      /* Remove a process from its wait queue */

/* Priority Adjustment */
//updatepri();    /* update priority */
//resetpri();     /* reset priority */
//setpri();       /* set priority */
//getpri();       /* get priority */

/* Dispatching */
//swtch();        /* swtich process */
//setrun();       /* set process running */
//wakeup();       /* wake up processes on chan */
//sleep();        /* set process to sleep till wakeup */
//endtsleep();    /* timeout process if still asleep or unsleep if stopped */
//tsleep();       /* General Sleep */
#endif /* _SYS_SCHED_EDF_H */
