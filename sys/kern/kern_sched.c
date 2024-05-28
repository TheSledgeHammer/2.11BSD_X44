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

#include <vm/include/vm.h>

static void		sched_edf_setup(struct sched_edf *, struct proc *);
static void		sched_cfs_setup(struct sched_cfs *, struct proc *);

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

/* return edf scheduler */
struct sched_edf *
sched_edf(sc)
	struct sched *sc;
{
	register struct sched_edf *edf;

	edf = sc->sc_edf;
	if (edf != NULL) {
		return (edf);
	}
	return (NULL);
}

/* return cfs scheduler */
struct sched_cfs *
sched_cfs(sc)
	struct sched *sc;
{
	register struct sched_cfs *cfs;

	cfs = sc->sc_cfs;
	if (cfs != NULL) {
		return (cfs);
	}
	return (NULL);
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

	sc->sc_utilrate = sched_rating(sc->sc_utilization);
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
	sched_set_slack(sc, p->p_cpticks, p->p_time, p->p_cpu); 								/* laxity/slack */
    sched_set_utilization(sc, p->p_cpu, p->p_swtime);                               		/* utilization */
    sched_set_demand(sc, p->p_time, p->p_cpticks, p->p_swtime, p->p_cpu);                   /* demand */
    sched_set_workload(sc, p->p_time, p->p_swtime, p->p_cpu);                            	/* workload */
    sched_set_priority_weighting(sc, p->p_pri, p->p_cpticks, sc->sc_slack, p->p_slptime);	/* priority weighting */
}

static int
sched_rate(val)
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
sched_weight(val)
	u_char val;
{
    if (SCHED_RATE_LOW(val)) {
        return (SCHED_WEIGHT_LOW);
    }
    if (SCHED_RATE_MEDIUM(val)) {
        return (SCHED_WEIGHT_MEDIUM);
    }
    if (SCHED_RATE_HIGH(val)) {
        return (SCHED_WEIGHT_HIGH);
    }
    return (-1);
}

static int
sched_rating(val)
	u_char val;
{
	int i, rate, weight;

	rate = sched_rate(val);
	weight = sched_weight(val);
	for (i = rate; i >= 0; i -= weight) {
		 if (sched_weight(i) != -1) {
			 weight = sched_weight(i);
			 return (weight);
		 }
	 }
	 return (-1);
}

void
sched_utilization_rate(sc)
	struct sched *sc;
{
	sc->sc_utilrate = sched_rating(sc->sc_utilization);
}

void
sched_demand_rate(sc)
	struct sched *sc;
{
	sc->sc_demandrate = sched_rating(sc->sc_demand);
}

void
sched_workload_rate(sc)
	struct sched *sc;
{
	sc->sc_workrate = sched_rating(sc->sc_workload);
}

int
sched_avg_rate(sc)
	struct sched *sc;
{
	sched_utilization_rate(sc);
	sched_demand_rate(sc);
	sched_workload_rate(sc);
	sc->sc_avgrate = ((sc->sc_utilrate + sc->sc_demandrate + sc->sc_workrate) / 3);
	return (sc->sc_avgrate);
}
