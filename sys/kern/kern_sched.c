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
 * The Global Scheduler Interface:
 * For interfacing different scheduling algorithms into the 2.11BSD Kernel
 * Should Primarily contain functions used in both the kernel & the various scheduler/s
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/sched.h>
#include <sys/sched_cfs.h>
#include <sys/sched_edf.h>
#include <sys/wait.h>

#include <vm/include/vm.h>

static void		sched_edf_setup(struct sched_edf *, struct proc *);
static void		sched_cfs_setup(struct sched_cfs *, struct proc *);
void 			sched_thread_avg_rate(struct sched *);
void			sched_thread_avg_weight(struct sched *);
void 			sched_nthreads(struct sched *);

void
sched_init(p)
	struct proc *p;
{
	register struct sched *sc;

	MALLOC(sc, struct sched *, sizeof(struct sched *), M_SCHED, M_WAITOK);

	p->p_sched = sc;
	sched_edf_setup(sc->sc_edf, p);
	sched_cfs_setup(sc->sc_cfs, p);
}

static void
sched_edf_setup(edf, p)
	struct sched_edf *edf;
	struct proc *p;
{
	if (edf == NULL) {
		MALLOC(edf, struct sched_edf *, sizeof(struct sched_edf *), M_SCHED, M_WAITOK);
	}

	edf->edf_cpticks = p->p_cpticks;
	edf->edf_pri = p->p_pri;
	edf->edf_cpu = p->p_cpu;
	edf->edf_time = p->p_time;
	edf->edf_slptime = p->p_slptime;
	edf->edf_release = p->p_swtime;
}

static void
sched_cfs_setup(cfs, p)
	struct sched_cfs *cfs;
	struct proc *p;
{
	if (cfs == NULL) {
		MALLOC(cfs, struct sched_cfs *, sizeof(struct sched_cfs *), M_SCHED, M_WAITOK);
	}

	RB_INIT(&cfs->cfs_parent);
	cfs->cfs_cpticks = p->p_cpticks;
	cfs->cfs_pri = p->p_pri;
	cfs->cfs_cpu = p->p_cpu;
	cfs->cfs_time = p->p_time;
	cfs->cfs_slptime = p->p_slptime;

	/* calculate cfs's dynamic variables */
	cfs->cfs_btl = BTL;
	cfs->cfs_bmg = BMG;
	cfs->cfs_bsched = BSCHEDULE;
}

/* set estcpu */
void
sched_estcpu(estcpu1, estcpu2)
	u_int estcpu1, estcpu2;
{
	u_int diff;

	if (estcpu1 != estcpu2) {
		if (estcpu1 > estcpu2) {
			diff = estcpu1 - estcpu2;
			if (diff >= 3) {
				estcpu1 = estcpu2;
			}
		} else if (estcpu1 < estcpu2) {
			diff = estcpu2 - estcpu1;
			if (diff >= 3) {
				estcpu2 = estcpu1;
			}
		}
	}
}

/* set cpticks */
void
sched_cpticks(cpticks1, cpticks2)
	int cpticks1, cpticks2;
{
	if (cpticks1 != cpticks2) {
		cpticks1 = cpticks2;
	}
}

void
sched_set_priority_weighting(sc, pri, deadline, slack, slptime)
	struct sched *sc;
	char deadline, slptime;
	u_char pri, slack;
{
	sc->sc_priweight = PRIORITY_WEIGHTING(pri, deadline, slack, slptime);
}

void
sched_set_slack(sc, deadline, timo, cost)
	struct sched *sc;
	char deadline, cost;
	u_char timo;
{
	sc->sc_slack = SLACK(deadline, timo, cost);
}

void
sched_set_utilization(sc, cost, release)
	struct sched *sc;
	char cost, release;
{
	sc->sc_utilization = UTILIZATION(cost, release);
}

void
sched_set_demand(sc, timo, deadline, release, cost)
	struct sched *sc;
	char timo, deadline, release, cost;
{
	sc->sc_demand = DEMAND(timo, deadline, release, cost);
}

void
sched_set_workload(sc, timo, release, cost)
	struct sched *sc;
	char timo, release, cost;
{
	sc->sc_workload = WORKLOAD(timo, release, cost);
}

void
sched_compute(sc, p)
	struct sched *sc;
	struct proc *p;
{
	/* laxity/slack */
	sched_set_slack(sc, p->p_cpticks, p->p_time, p->p_cpu);
	/* utilization */
    sched_set_utilization(sc, p->p_cpu, p->p_swtime);
    /* demand */
    sched_set_demand(sc, p->p_time, p->p_cpticks, p->p_swtime, p->p_cpu);
	/* workload */
    sched_set_workload(sc, p->p_time, p->p_swtime, p->p_cpu);
	/* priority weighting */
    sched_set_priority_weighting(sc, p->p_pri, p->p_cpticks, sc->sc_slack, p->p_slptime);
    /* thread average rate */
    sched_thread_avg_rate(sc);
    /* thread average weight */
    sched_thread_avg_weight(sc);
    /* optimal nthreads for process */
    sched_nthreads(sc);
}

static int
sched_thread_rate(val)
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

static int
sched_thread_weight(val)
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

#define sched_thread_utilization_rate(sc)		((sc)->sc_utilrate = sched_thread_rate((sc)->sc_utilization))
#define sched_thread_demand_rate(sc)			((sc)->sc_demandrate = sched_thread_rate((sc)->sc_demand))
#define sched_thread_workload_rate(sc)			((sc)->sc_workrate = sched_thread_rate((sc)->sc_workload))

#define sched_thread_utilization_weight(sc)		((sc)->sc_utilweight = sched_thread_weight((sc)->sc_utilization))
#define sched_thread_demand_weight(sc)			((sc)->sc_demandweight = sched_thread_weight((sc)->sc_demand))
#define sched_thread_workload_weight(sc)		((sc)->sc_workweight = sched_thread_weight((sc)->sc_workload))

void
sched_thread_avg_rate(sc)
	struct sched *sc;
{
    sched_thread_utilization_rate(sc);
    sched_thread_demand_rate(sc);
    sched_thread_workload_rate(sc);
    sc->sc_avgrate = ((sc->sc_utilrate + sc->sc_demandrate + sc->sc_workrate) / 3);
}

void
sched_thread_avg_weight(struct sched *sc)
{
    sched_thread_utilization_weight(sc);
    sched_thread_demand_weight(sc);
    sched_thread_workload_weight(sc);
    sc->sc_avgweight = ((sc->sc_utilweight + sc->sc_demandweight + sc->sc_workweight) / 3);
}

void
sched_nthreads(sc)
	struct sched *sc;
{
	int nthreads;
	int i, r, w;

    r = sc->sc_avgrate;
    w = sc->sc_avgweight;
    nthreads = 0;
    for (i = r; i >= 0; i -= w) {
    	nthreads++;
        if (sched_thread_weight(i) != -1) {
            w = sched_thread_weight(i);
        }
    }
    sc->sc_optnthreads = nthreads;
}

void
sched_check_threads(sc, p)
	struct sched *sc;
	struct proc *p;
{
	if (sc->sc_optnthreads > 0) {
		if (p->p_nthreads < sc->sc_optnthreads) {
			p->p_flag |= P_TDCREATE;
		}
		if (p->p_nthreads > sc->sc_optnthreads) {
			p->p_flag |= P_TDDESTROY;
		}
	}
}

/*
 * TODO:
 * Implement use of thread stealing
 */
/*
 * proc_create_nthreads: [internal use only]
 * create nthreads on existing proc.
 * Note: nthreads is pre-determined by "sched_nthreads" and "sched_check_threads".
 */
int
proc_create_nthreads(p)
	struct proc *p;
{
	struct thread *td;
	int nthreads, error, i;

	if ((p->p_flag & P_TDCREATE) == 0) {
		nthreads = p->p_sched->sc_optnthreads - p->p_nthreads;
		for (i = 0; i < nthreads; i++) {
			error = newthread(&td, NULL, THREAD_STACK, FALSE);
			if (error != 0) {
				return (error);
			}
		}
		p->p_flag &= ~P_TDCREATE;
	}
	return (0);
}

/*
 * proc_destroy_nthreads: [internal use only]
 * destroy nthreads on existing proc.
 * Note: nthreads is pre-determined by "sched_nthreads" and "sched_check_threads".
 */
void
proc_destroy_nthreads(p, ecode)
	struct proc *p;
	int ecode;
{
	register struct thread *td;
	int nthreads, i;

	if ((p->p_flag & P_TDDESTROY) == 0) {
		nthreads = p->p_nthreads - p->p_sched->sc_optnthreads;
		for (i = 0; i < nthreads; i++) {
			thread_exit(W_EXITCODE(ecode, 0));
		}
		p->p_flag &= ~P_TDDESTROY;
	}
}
