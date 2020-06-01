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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sched.h>

void
sched_setup(sd, p)
	struct sched *sd;
	struct proc *p;
{
	MALLOC(sd, struct sched *, sizeof(struct sched *), M_GSCHED, M_WAITOK);
	TAILQ_INIT(&sd->sc_header);
	sd->sc_proc = p;
	sd->sc_pri = p->p_pri;
	sd->sc_cpu = p->p_cpu;
	sd->sc_time = p->p_time;
	sd->sc_nice = p->p_nice;
	sd->sc_slptime = p->p_slptime;

	sd->sc_release = 0;
	sd->sc_deadline = 0;
	sd->sc_remtime = 0;

	/* EDF Calc priweight */
	//sd->sc_priweight = setpriweight(p->p_pri, x (deadline), y (release), p->p_slptime);
}

void
sched_edf_setup(sd, edf)
	struct sched *sd;
	struct sched_edf *edf;
{
	MALLOC(edf, struct sched_edf *, sizeof(struct sched_edf *), M_SCHEDEDF, M_WAITOK);
	sd->sc_edf = edf;
}

void
sched_cfs_setup(sd, cfs)
	struct sched *sd;
	struct sched_cfs *cfs;
{
	MALLOC(cfs, struct sched_cfs *, sizeof(struct sched_cfs *), M_SCHEDCFS, M_WAITOK);
	sd->sc_cfs = cfs;
}

/* insert the proc run-queue into sched list if not null */
void
sched_enqueue(sd, p)
	struct sched *sd;
	struct proc *p;
{
	struct proc *pq = getrq(p);

	if(pq == p) {
		TAILQ_INSERT_HEAD(&sd->sc_header, pq, p_sc_entry);
	}
}

/* remove the proc run-queue from sched list if null */
void
sched_dequeue(sd, p)
	struct sched *sd;
	struct proc *p;
{
	struct proc *pq = getrq(p);
	if(pq == p) {
		TAILQ_REMOVE(&sd->sc_header, pq, p_sc_entry);
	}
}

/* search proc run-queue for global sched list entry */
struct proc *
sched_getproc(sd, p)
	struct sched *sd;
	struct proc *p;
{
	struct proc *pq = getrq(p);
	TAILQ_FOREACH(pq, &sd->sc_header, p_sc_entry) {
		if (TAILQ_NEXT(pq, p_sc_entry) == p) {
			return (p);
		}
	}
	return (NULL);
}

/* Negative means a higher weighting */
int
setpriweight(pwp, pwd, pwr, pws)
	float pwp, pwd, pwr, pws;
{
	int pw_pri = PW_FACTOR(pwp, PW_PRIORITY);
	int pw_dead = PW_FACTOR(pwd, PW_DEADLINE);
	int pw_rel = PW_FACTOR(pwr, PW_RELEASE);
	int pw_slp = PW_FACTOR(pws, PW_SLEEP);

	int priweight = ((pw_pri + pw_dead + pw_rel + pw_slp) / 4);

	if(priweight > 0) {
		priweight = priweight * - 1;
	}

	return (priweight);
}
