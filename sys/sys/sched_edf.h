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

#ifndef _SYS_GSCHED_EDF_H
#define _SYS_GSCHED_EDF_H

#include <sys/sched.h>

struct gsched_edf {
	struct gsched 		*edf_gsched;		/* pointer to global scheduler */

	struct proc 		*edf_rqlink;		/* pointer to linked list of running processes */

	struct proc			*edf_proc;

    int	    			edf_flag;			/* P_* flags. */
    char    			edf_stat;			/* S* process status. */

    u_int   			edf_estcpu;			/* Time averaged value of p_cpticks. */
    int					edf_cpticks;		/* Ticks of cpu time. */

    u_char  			edf_pri;			/* Process  priority, negative is high */
    u_char  			edf_cpu;			/* cpu usage for scheduling */
    u_char  			edf_time;			/* resident time for scheduling */
    char    			edf_slptime;		/* Time since last blocked. secs sleeping */

    u_char 				edf_slack;			/* slack / laxity time */
    char				edf_release;		/* time till release from current block. see above */
    int					edf_priweight;		/* priority weighting (calculated from various factors) */

    char 				edf_delta; 			/* Inherited Deadline (UN-USED) */
    u_char 				edf_remtime; 		/* time remaining (UN-USED) */
};

#define P_EDFFAIL 		0x8000	/* Failed EDF Test */
#define P_EDFPREEMPT 	0x16000 /* Preemption Flag: Suggest to CFS to preempt this process */

u_char 	edf_slack(char, u_char, char);
int 	edf_utilization(char, char);
int 	edf_demand(char, char, char, char);
int 	edf_workload(char, char, char);
int 	edf_test(struct gsched_edf *);
int		edf_schedcpu(struct proc *);


/* Basic Concept */
/*
- Generalised routines as per kern_synch
- Scheduler expands on those rountines, in a pluggable manner
- All current rountines in kern_synch are generic for proc
- edf goals: maintain and prioritise the run queues, pass of to cfs
- cfs goals: Equal time, must adhere to edf's priorities (priority weighting). But providing fair time.
    Incomplete jobs are passed back to edf after x time or completed CFS loop
Multiple CFS?
 - Each priority queue runs its own CFS
    - Different sized priority queues?
    &/ OR
    - Different Time Slices (Could be calculated from EDF)?

 Start-up:
 - edf must start & have process in run-queue before cfs can start (rqinit)
*/
#endif /* _SYS_GSCHED_EDF_H */
