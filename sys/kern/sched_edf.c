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
 * Global Scheduler's: Earliest Deadline First Scheduling Algorithm.
 * Designed to schedule per single run-queue.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/sched.h>
#include <sys/sched_edf.h>

#include <vm/include/vm.h>

#include <machine/cpu.h>

void
edf_set_priority_weighting(edf, pri, deadline, slack, slptime)
	struct gsched_edf *edf;
	char deadline, slptime;
	u_char pri, slack;
{
	edf->edf_priweight = PRIORITY_WEIGHTING(pri, deadline, slack, slptime);
}

void
edf_set_slack(edf, deadline, timo, cost)
	struct gsched_edf *edf;
	char deadline, cost;
	u_char timo;
{
	edf->edf_slack = SLACK(deadline, timo, cost);
}

void
edf_set_utilization(edf, cost, release)
	struct gsched_edf *edf;
	char cost, release;
{
	edf->edf_utilization = UTILIZATION(cost, release);
}

void
edf_set_demand(edf, timo, deadline, release, cost)
	struct gsched_edf *edf;
	char timo, deadline, release, cost;
{
	edf->edf_demand = DEMAND(timo, deadline, release, cost);
}

void
edf_set_workload(edf, timo, release, cost)
	struct gsched_edf *edf;
	char timo, release, cost;
{
	edf->edf_workload = WORKLOAD(timo, release, cost);
}

/* CPU Utilization per task (U <= 1) */
int
edf_test_utilization(cost, release)
	char cost, release;
{
	u_char U = UTILIZATION(cost, release);
	if(U <= 1) { 							/* test if can be scheduled */
		return (0);
	}
	return (1);
}

/* CPU Demand: total amount of cpu demand per task (H[t], t = time) */
int
edf_test_demand(timo, deadline, release, cost)
	char timo, deadline, release, cost;
{
	u_char H = DEMAND(timo, deadline, release, cost);
	if(H >= 0 && H <= timo) {				/* test if can be scheduled */
		return (0);
	}
	return (1);
}

/* CPU Workload: accumulative amount of cpu time for all tasks (W[t], t = time) */
int
edf_test_workload(timo, release, cost)
	char timo, release, cost;
{
	u_char W = WORKLOAD(timo, release, cost);
	if(W >= 0 && W <= timo) {				/* test if can be scheduled */
		return (0);
	}
	return (1);
}

void
edf_compute(edf)
	struct gsched_edf *edf;
{
    edf_set_slack(edf, edf->edf_cpticks, edf->edf_time, edf->edf_cpu); 									        /* laxity/slack */
    edf_set_utilization(edf, edf->edf_cpu, edf->edf_release);                                                   /* utilization */
    edf_set_demand(edf, edf->edf_time, edf->edf_cpticks, edf->edf_release, edf->edf_cpu);                       /* demand */
    edf_set_workload(edf, edf->edf_time, edf->edf_release, edf->edf_cpu);                                       /* workload */
    edf_set_priority_weighting(edf, edf->edf_pri, edf->edf_cpticks, edf->edf_slack, edf->edf_slptime);			/* priority weighting */
}

int
edf_test(edf)
	struct gsched_edf *edf;
{
	/* check deadline is possible in given time */
	/*
	if(edf->edf_time < edf->edf_cpticks) {
		edf->edf_release = 0;
		goto error;
	}
	*/
	/* Sanity Check */
	if(edf->edf_time == 0) {
		printf("time not set");
		goto error;
	}
	if(edf->edf_cpu == 0) {
		printf("cost not set");
		goto error;
	}
	if(edf->edf_cpticks == 0) {
		edf->edf_cpticks = edf->edf_time;
		goto error;
	}
	if(edf->edf_cpu > edf->edf_cpticks) {
		printf("cost > deadline");
		goto error;
	}
    edf->edf_release = edf->edf_time;

	/* test cpu utilization, demand & workload, can be scheduled */
	if (edf_test_utilization(edf->edf_cpu, edf->edf_release) != 0) {
		goto error;
	}

	if (edf_test_demand(edf->edf_time, edf->edf_cpticks, edf->edf_release, edf->edf_cpu) != 0) {
		goto error;
	}

	if (edf_test_workload(edf->edf_time, edf->edf_release, edf->edf_cpu) != 0) {
		goto error;
	}

	printf("is schedulable");
	return (0);

error:
	printf("is not schedulable");
	return (1);
}

/* Returns once complete or an error occured */
int
edf_schedcpu(p)
	struct proc *p;
{
	register struct gsched *gsched = p->p_gsched;
	register struct gsched_edf *edf = gsched_edf(gsched);
	int error;

	edf_compute(edf);
	gsched->gsc_slack = edf->edf_slack;
	gsched->gsc_utilization = edf->edf_utilization;
	gsched->gsc_demand = edf->edf_demand;
	gsched->gsc_workload = edf->edf_workload;
	gsched->gsc_priweight = edf->edf_priweight;
	p->p_gsched = gsched;

	if (edf_test(edf)) {
		return (0);
	} else {
		panic("edf_test failed");
		reschedule(p);
		error = P_EDFFAIL;
	}
	return (error);
}
