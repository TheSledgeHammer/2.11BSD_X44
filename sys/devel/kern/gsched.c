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

#include "sys/gsched.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/queue.h>

/* The Global Scheduler Interface: For interfacing different scheduling algorithms into the 2.11BSD Kernel */
/* Primarily for functions that will be used within both the kernel & the scheduler/s */

void
gsched_init(p)
	struct proc *p;
{
	struct gsched *gsd = gsched_setup(p);
	gsched_edf_setup(gsd, p);
	gsched_cfs_setup(gsd, p);
	//simple_lock_int(&gsd->gsc_lock); /* Unused */
}

struct gsched *
gsched_setup(p)
	struct proc *p;
{
	struct gsched *gsd = p->p_gsched;
	if (gsd == NULL) {
		MALLOC(gsd, struct gsched *, sizeof(struct gsched *), M_GSCHED, M_WAITOK);
	}
	gsd->gsc_proc = p;
	return (gsd);
}

void
gsched_edf_setup(gsd, p)
	struct gsched *gsd;
	struct proc *p;
{
	register struct gsched_edf *edf = gsched_edf(gsd);

	if (edf == NULL) {
		MALLOC(edf, struct gsched_edf *, sizeof(struct gsched_edf *), M_GSCHED, M_WAITOK);
	}

	edf->edf_proc = p;
	edf->edf_pri = p->p_pri;
	edf->edf_cpu = p->p_cpu;
	edf->edf_time = p->p_time;
	edf->edf_slptime = p->p_slptime;
}

void
gsched_cfs_setup(gsd, p)
	struct gsched *gsd;
	struct proc *p;
{
	register struct gsched_cfs *cfs = gsched_cfs(gsd);

	if (cfs == NULL) {
		MALLOC(cfs, struct gsched_cfs *, sizeof(struct gsched_cfs *), M_GSCHED, M_WAITOK);
	}

	RB_INIT(cfs->cfs_parent);
	cfs->cfs_proc = p;
	cfs->cfs_pri = p->p_pri;
	cfs->cfs_cpu = p->p_cpu;
	cfs->cfs_time = p->p_time;
	cfs->cfs_slptime = p->p_slptime;
}

/* return edf scheduler */
struct gsched_edf *
gsched_edf(gsd)
	struct gsched *gsd;
{
	return (gsd->gsc_edf);
}

/* return cfs scheduler */
struct gsched_cfs *
gsched_cfs(gsd)
	struct gsched *gsd;
{
	return (gsd->gsc_cfs);
}

/* Return difference between time and estcpu */
u_char
gsched_timediff(time, estcpu)
	u_char time;
	u_int estcpu;
{
	u_char diff;
	if (time > estcpu) {
		diff = time - estcpu;
	} else {
		diff = estcpu - time;
	}
	return (diff);
}

/* a priority weighting, dependent on various factors */
int
gsched_setpriweight(pwp, pwd, pwr, pws)
	int pwp, pwd, pwr, pws;
{
	int pw_pri = PW_FACTOR(pwp, PW_PRIORITY);
	int pw_dead = PW_FACTOR(pwd, PW_DEADLINE);
	int pw_rel = PW_FACTOR(pwr, PW_RELEASE);
	int pw_slp = PW_FACTOR(pws, PW_SLEEP);

	int priweight = ((pw_pri + pw_dead + pw_rel + pw_slp) / 4);

	return (priweight);
}

void
gsched_lock(p)
	struct proc *p;
{
	simple_lock(p->p_gsched->gsc_lock);
}

void
gsched_unlock(p)
	struct proc *p;
{
	simple_unlock(p->p_gsched->gsc_lock);
}
