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

#include "sys/gsched_edf.h"

/* return slack/laxity time */
u_char
edf_slack(deadline, time, cost)
	char deadline, cost;
	u_char time;
{
	return ((deadline - time) - cost);
}

/* CPU Utilization per task (U <= 1) */
int
edf_utilization(release, cost)
	char release, cost;
{
	u_char U = (cost / release);
	if(U <= 1) { 							/* test if can be scheduled */
		return (0);
	}
	return (1);
}

/* CPU Demand: total amount of cpu demand per task (H[t], t = time) */
int
edf_demand(time, deadline, release, cost)
	char time, deadline, release, cost;
{
	u_char N = (time - deadline + release);
	u_char H = (N * cost) / release;
	if(H >= 0 && H <= time) {				/* test if can be scheduled */
		return (0);
	}
	return (1);
}

/* CPU Workload: accumulative amount of cpu time for all tasks (W[t], t = time) */
int
edf_workload(time, release, cost)
	char time, release, cost;
{
	u_char N = (time / release);
	u_char W = (N * cost);
	if(W >= 0 && W <= time) {				/* test if can be scheduled */
		return (0);
	}
	return (1);
}

void
edf_testrq(p)
	struct proc *p;
{
	struct gsched_edf *edf = gsched_edf(p->p_gsched);

	/* check deadline is possible in given time */
	if(edf->edf_time < edf->edf_cpticks) {
		edf->edf_release = 0;
		goto error;
	}
    edf->edf_release = edf->edf_time;

    /* get laxity/slack */
    edf->edf_slack = edf_slack(edf->edf_cpticks, edf->edf_time, edf->edf_cpu);

	/* check linked list that running processes are not empty */
	edf->edf_rqlink = getrq(p);
	if(edf->edf_rqlink == NULL) {
		goto error;
	} else {
		/* set edf proc to edf run-queue */
		edf->edf_proc = edf->edf_rqlink;
		/* test cpu utilization, demand & workload can be scheduled */
		if (edf_utilization(edf->edf_release, edf->edf_cpu)) {
			printf("utilization is schedulable");
		} else if (edf_demand(edf->edf_time, edf->edf_cpticks, edf->edf_release, edf->edf_cpu)) {
			printf("demand is schedulable");
		} else if (edf_workload(edf->edf_time, edf->edf_release, edf->edf_cpu)) {
			printf("workload is schedulable");
		}
	}
	p->p_gsched->gsc_priweight = gsched_setpriweight(edf->edf_pri, edf->edf_cpticks, edf->edf_slack, edf->edf_slptime);

error:
	printf(" is likely not schedulable");
}

edf_sanity_check(edf)
	struct gsched_edf *edf;
{
	if(edf->edf_time == 0) {
		printf("time not set");
	}
	if(edf->edf_cpu == 0) {
		printf("cost not set");
	}
	if(edf->edf_cpticks > edf->edf_time) {
		printf("deadline > time");
	}
	if(edf->edf_cpticks == 0) {
		edf->edf_cpticks = edf->edf_time;
	}
	if(edf->edf_cpu > edf->edf_cpticks) {
		printf("cost > deadline");
	}
}
