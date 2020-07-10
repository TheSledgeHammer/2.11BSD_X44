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

void
gsched_init(p)
	struct proc *p;
{
	register struct gsched *gsd = gsched_setup(p);
	gsched_edf_setup(gsd);
	gsched_edf_setup(gsd);
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
	gsd->gsc_pri = p->p_pri;
	gsd->gsc_cpu = p->p_cpu;
	gsd->gsc_time = p->p_time;
	gsd->gsc_slptime = p->p_slptime;

	return (gsd);
}

void
gsched_edf_setup(gsd)
	struct gsched *gsd;
{
	register struct gsched_edf *edf = gsched_edf(gsd);
	if (edf == NULL) {
		MALLOC(edf, struct gsched_edf *, sizeof(struct gsched_edf *), M_GSCHED, M_WAITOK);
	}

	edf->edf_proc = gsd->gsc_proc;
	edf->edf_pri = gsd->gsc_pri;
	edf->edf_cpu = gsd->gsc_cpu;
	edf->edf_time = gsd->gsc_time;
	edf->edf_slptime = gsd->gsc_slptime;
}

void
gsched_cfs_setup(gsd)
	struct gsched *gsd;
{
	register struct gsched_cfs *cfs = gsched_cfs(gsd);
	if (cfs == NULL) {
		MALLOC(cfs, struct gsched_cfs *, sizeof(struct gsched_cfs *), M_GSCHED, M_WAITOK);
	}

	RB_INIT(cfs->cfs_parent);
	cfs->cfs_proc = gsd->gsc_proc;
	cfs->cfs_pri = gsd->gsc_pri;
	cfs->cfs_cpu = gsd->gsc_cpu;
	cfs->cfs_time = gsd->gsc_time;
	cfs->cfs_slptime = gsd->gsc_slptime;
}

void
gsched_setrq(gsd)
	struct gsched *gsd;
{
	setrq(gsd->gsc_proc);
}

void
gsched_remrq(gsd)
	struct gsched *gsd;
{
	remrq(gsd->gsc_proc);
}

struct proc *
gsched_getrq(gsd)
	struct gsched *gsd;
{
	return (getrq(gsd->gsc_proc));
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

/* Negative means a higher priority weighting */
int
gsched_setpriweight(pwp, pwd, pwr, pws)
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

void
gsched_lock(gsd)
	struct gsched *gsd;
{
	simple_lock(gsd->gsc_lock);
}

void
gsched_unlock(gsd)
	struct gsched *gsd;
{
	simple_unlock(gsd->gsc_lock);
}

/* Greatest Common Divisor: Unused */
int
gsched_gcd(a, b)
	int a, b;
{
	if(b == 0) {
		return (a);
	} else {
		return (gcd(b, a % b));
	}
}

/* Least Common Multiple: Unused */
int
gsched_lcm(a, b)
	int a, b;
{
	return ((a * b) / gcd(a, b));
}
