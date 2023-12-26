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

struct sched 	*sched_setup(struct proc *);
static void		sched_edf_setup(struct sched *, struct proc *);
static void		sched_cfs_setup(struct sched *, struct proc *);

void
sched_init(p)
	struct proc *p;
{
	struct sched *gsd;
	gsd = sched_setup(p);
	sched_edf_setup(gsd, p);
	sched_cfs_setup(gsd, p);
}

struct sched *
sched_setup(p)
	struct proc *p;
{
	struct sched *gsd = p->p_sched;

	if (gsd == NULL) {
		MALLOC(gsd, struct sched *, sizeof(struct sched *), M_SCHED, M_WAITOK);
	}

	gsd->gsc_proc = p;
	return (gsd);
}

static void
sched_edf_setup(gsd, p)
	struct sched *gsd;
	struct proc *p;
{
	register struct sched_edf *edf = sched_edf(gsd);

	if (edf == NULL) {
		MALLOC(edf, struct sched_edf *, sizeof(struct sched_edf *), M_GCHED, M_WAITOK);
	}

	edf->edf_proc = p;
	edf->edf_cpticks = p->p_cpticks;
	edf->edf_pri = p->p_pri;
	edf->edf_cpu = p->p_cpu;
	edf->edf_time = p->p_time;
	edf->edf_slptime = p->p_slptime;
}

static void
sched_cfs_setup(gsd, p)
	struct sched *gsd;
	struct proc *p;
{
	register struct sched_cfs *cfs = sched_cfs(gsd);

	if (cfs == NULL) {
		MALLOC(cfs, struct sched_cfs *, sizeof(struct sched_cfs *), M_SCHED, M_WAITOK);
	}

	RB_INIT(&cfs->cfs_parent);
	cfs->cfs_proc = p;
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
sched_edf(gsd)
	struct sched *gsd;
{
	return (gsd->gsc_edf);
}

/* return cfs scheduler */
struct sched_cfs *
sched_cfs(gsd)
	struct sched *gsd;
{
	return (gsd->gsc_cfs);
}

/* set estcpu */
void
sched_estcpu(estcpu1, estcpu2)
	u_int estcpu1, estcpu2;
{
	u_int diff;
	if(estcpu1 != estcpu2) {
		if(estcpu1 > estcpu2) {
			diff = estcpu1 - estcpu2;
			if(diff >= 3) {
				estcpu1 = estcpu2;
			}
		} else if(estcpu1 < estcpu2) {
			diff = estcpu2 - estcpu1;
			if(diff >= 3) {
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
	if(cpticks1 != cpticks2) {
		cpticks1 = cpticks2;
	}
}
