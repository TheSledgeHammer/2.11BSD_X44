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

static void	sched_edf_init(struct sched_edf *, struct proc *);
static void	sched_cfs_init(struct sched_cfs *, struct proc *);
void 		sched_factor_compute(struct sched_factor *, u_char, u_char, u_char);
void 		sched_factor_utilization(struct sched_factor *, u_char);
void 		sched_factor_demand(struct sched_factor *, u_char);
void 		sched_factor_workload(struct sched_factor *, u_char);
void 		sched_factor_average(struct sched_factor *);
void 		sched_factor_nthreads(struct sched_factor *);

void
sched_init(p)
	struct proc *p;
{
	register struct sched *sc;

	MALLOC(sc, struct sched *, sizeof(struct sched *), M_SCHED, M_WAITOK);

	p->p_sched = sc;
	sched_edf_init(sc->sc_edf, p);
	sched_cfs_init(sc->sc_cfs, p);
}

static void
sched_edf_init(edf, p)
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
sched_cfs_init(cfs, p)
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

/*
 * set estcpu
 */
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

/*
 * set cpticks
 */
void
sched_cpticks(cpticks1, cpticks2)
	int cpticks1, cpticks2;
{
	if (cpticks1 != cpticks2) {
		cpticks1 = cpticks2;
	}
}

void
sched_base_priority_weighting(sc, pri, deadline, slack, slptime)
	struct sched *sc;
	char deadline, slptime;
	u_char pri, slack;
{
	sc->sc_priweight = PRIORITY_WEIGHTING(pri, deadline, slack, slptime);
}

void
sched_base_slack(sc, deadline, timo, cost)
	struct sched *sc;
	char deadline, cost;
	u_char timo;
{
	sc->sc_slack = SLACK(deadline, timo, cost);
}

void
sched_base_utilization(sc, cost, release)
	struct sched *sc;
	char cost, release;
{
	sc->sc_utilization = UTILIZATION(cost, release);
}

void
sched_base_demand(sc, timo, deadline, release, cost)
	struct sched *sc;
	char timo, deadline, release, cost;
{
	sc->sc_demand = DEMAND(timo, deadline, release, cost);
}

void
sched_base_workload(sc, timo, release, cost)
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
	sched_base_slack(sc, p->p_cpticks, p->p_time, p->p_cpu);
	/* utilization */
	sched_base_utilization(sc, p->p_cpu, p->p_swtime);
	/* demand */
	sched_base_demand(sc, p->p_time, p->p_cpticks, p->p_swtime, p->p_cpu);
	/* workload */
	sched_base_workload(sc, p->p_time, p->p_swtime, p->p_cpu);
	/* priority weighting */
	sched_base_priority_weighting(sc, p->p_pri, p->p_cpticks, sc->sc_slack,
			p->p_slptime);
	/* schedule factors to compute */
	sched_factor_compute(sc->sc_factor, sc->sc_utilization, sc->sc_demand,
			sc->sc_workload);
}

/* schedule factor */

void
sched_factor_compute(sf, utilization, demand, workload)
	struct sched_factor *sf;
	u_char utilization;
	u_char demand;
	u_char workload;
{
	if (sf == NULL) {
		MALLOC(sf, struct sched_factor *, sizeof(struct sched_factor *), M_SCHED, M_WAITOK);
	}

	/* factors utilization rate and weight */
	sched_factor_utilization(sf, utilization);
	/* factors demand rate and weight */
	sched_factor_demand(sf, demand);
	/* factors workload rate and weight */
	sched_factor_workload(sf, workload);
	/* factors average rate and weight */
	sched_factor_average(sf);
	/* factor optimal nthreads for process */
	sched_factor_nthreads(sf);
}

#define SCHED_AVG(a, b, c) (((a)+(b)+(c))/3)

/*
 * schedule utilization
 */
void
sched_factor_utilization(sf, utilization)
	struct sched_factor *sf;
	u_char utilization;
{
	sf->sfu_utilization = utilization;
	sf->sfu_rate = sched_rating(sf->sfu_utilization);
	sf->sfu_weight = sched_weighting(sf->sfu_utilization);
}

/*
 * schedule demand
 */
void
sched_factor_demand(sf, demand)
	struct sched_factor *sf;
	u_char demand;
{
	sf->sfd_demand = demand;
    sf->sfd_rate = sched_rating(sf->sfd_demand);
    sf->sfd_weight = sched_weighting(sf->sfd_demand);
}

/*
 * schedule workload
 */
void
sched_factor_workload(sf, workload)
	struct sched_factor *sf;
	u_char workload;
{
	sf->sfw_workload = workload;
	sf->sfw_rate = sched_rating(sf->sfw_workload);
	sf->sfw_weight = sched_weighting(sf->sfw_workload);
}

/*
 * schedule average:
 * Calculated from below;
 * - utilization: rate & weight
 * - demand: rate & weight
 * - workload: rate & weight
 */
void
sched_factor_average(sf)
	struct sched_factor *sf;
{
	sf->sf_avgrate = SCHED_AVG(sf->sfu_rate, sf->sfd_rate, sf->sfw_rate);
	sf->sf_avgweight = SCHED_AVG(sf->sfu_weight, sf->sfd_weight, sf->sfw_weight);
}

/*
 * Calculate the optimal number of threads
 * from the average rate and weight.
 */
void
sched_factor_nthreads(sf)
	struct sched_factor *sf;
{
	int nthreads, factor, i;
	int rate, weight;

	rate = sf->sf_avgrate;
	weight = sf->sf_avgweight;
	nthreads = 0;
	for (i = rate; i >= 0; i -= weight) {
		nthreads++;
		factor = sched_weighting(i);
		if (factor != -1) {
			weight = sched_weighting(i);
		}
	}
	sf->sf_optnthreads = nthreads;
}

/*
 * Determine how many additional threads proc needs
 * from the optimal number of threads and the current
 * number of threads.
 */
void
sched_check_threads(sc, p)
	struct sched *sc;
	struct proc *p;
{
	struct sched_factor *sf;

	sf = sc->sc_factor;
	if (sf->sf_optnthreads > 0) {
		if (p->p_nthreads < sf->sf_optnthreads) {
			p->p_flag |= P_TDCREATE;
		}
		if (p->p_nthreads > sf->sf_optnthreads) {
			p->p_flag |= P_TDDESTROY;
		}
	}
}

/*
 * sched_create_threads:
 * - create nthreads on a new or existing process.
 * Note: nthreads is pre-determined by "sched_nthreads" and "sched_check_threads".
 */
int
sched_create_threads(newpp, newtd, name, stack)
	struct proc **newpp;
	struct thread **newtd;
	char *name;
	size_t stack;
{
	struct proc *pp;
	struct thread *tdp;
	struct sched *sc;
	struct sched_factor *sf;
	bool_t forkproc;
	int i, error, nthreads;

	pp = *newpp;
	tdp = *newtd;
	sc = pp->p_sched;
	sf = sc->sc_factor;
	if ((pp->p_flag & P_TDCREATE) == 0) {
		nthreads = sf->sf_optnthreads;
		for (i = 0; i < nthreads + 1; i++) {
			if ((tdp->td_flag & TD_STEALABLE) == 0) {
				thread_steal(pp, tdp);
			} else {
				/* check proc has any threads ready */
				if ((pp->p_tqsready > 0) && (pp->p_stat != SRUN)) {
					forkproc = TRUE;
				} else {
					forkproc = FALSE;
				}
				error = newthread1(&pp, &tdp, name, stack, forkproc);
				if (error != 0) {
					return (error);
				}
			}
		}
		pp->p_flag &= ~P_TDCREATE;
	}
	*newpp = pp;
	*newtd = tdp;
	return (0);
}

/*
 * sched_destroy_threads:
 * - destroy nthreads on an existing process.
 * Note: nthreads is pre-determined by "sched_nthreads" and "sched_check_threads".
 */
void
sched_destroy_threads(pp, ecode, all)
	struct proc *pp;
	int ecode, all;
{
	struct sched *sc;
	struct sched_factor *sf;
	int i, nthreads;

	sc = pp->p_sched;
	sf = sc->sc_factor;
	if ((pp->p_flag & P_TDDESTROY) == 0) {
		nthreads = sf->sf_optnthreads;
		for (i = 0; i < nthreads + 1; i++) {
			thread_exit(W_EXITCODE(ecode, 0), all);
		}
		pp->p_flag &= ~P_TDDESTROY;
	}
}
