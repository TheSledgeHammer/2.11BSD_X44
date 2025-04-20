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
edf_compute(edf, p)
	struct sched_edf *edf;
	struct proc *p;
{
	edf->edf_cpticks = p->p_cpticks;
	edf->edf_pri = p->p_pri;
	edf->edf_cpu = p->p_cpu;
	edf->edf_time = p->p_time;
	edf->edf_slptime = p->p_slptime;
	edf->edf_release = p->p_swtime;
}

int
edf_test(edf)
	struct sched_edf *edf;
{
	/* Sanity Check */
	if(edf->edf_time == 0) {
		printf("edf_test: time not set");
		goto error;
	}
	if(edf->edf_cpu == 0) {
		printf("edf_test: cost not set");
		goto error;
	}
	if(edf->edf_cpticks == 0) {
		edf->edf_cpticks = edf->edf_time;
		goto error;
	}
	if(edf->edf_cpu > edf->edf_cpticks) {
		printf("edf_test: cost > deadline");
		goto error;
	}

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

	printf("edf_test: proc is schedulable");
	return (0);

error:
	printf("edf_test: proc is not schedulable");
	return (1);
}

/* Returns once complete or an error occured */
int
edf_schedcpu(p)
	struct proc *p;
{
	register struct sched *sc;
	register struct sched_edf *edf;
	int error;

	sc = p->p_sched;
	edf = sc->sc_edf;
	edf_compute(edf, p);
	sched_compute(sc, p);
	edf->edf_slack = sc->sc_slack;
	edf->edf_priweight = sc->sc_priweight;

	if (edf_test(edf)) {
		sched_check_threads(sc, p);
		error = 0;
	} else {
		//reschedule(p);
		error = -1;
	}
	return (error);
}
