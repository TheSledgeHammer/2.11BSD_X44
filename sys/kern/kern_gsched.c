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
#include <sys/gsched.h>
#include <sys/gsched_cfs.h>
#include <sys/gsched_edf.h>

#include <vm/include/vm.h>

struct gsched 	*gsched_setup(struct proc *);
static void		gsched_edf_setup(struct gsched *, struct proc *);
static void		gsched_cfs_setup(struct gsched *, struct proc *);

void
gsched_init(p)
	struct proc *p;
{
	struct gsched *gsd;
	gsd = gsched_setup(p);
	gsched_edf_setup(gsd, p);
	gsched_cfs_setup(gsd, p);
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

static void
gsched_edf_setup(gsd, p)
	struct gsched *gsd;
	struct proc *p;
{
	register struct gsched_edf *edf = gsched_edf(gsd);

	if (edf == NULL) {
		MALLOC(edf, struct gsched_edf *, sizeof(struct gsched_edf *), M_GSCHED, M_WAITOK);
	}

	edf->edf_proc = p;
	edf->edf_cpticks = p->p_cpticks;
	edf->edf_pri = p->p_pri;
	edf->edf_cpu = p->p_cpu;
	edf->edf_time = p->p_time;
	edf->edf_slptime = p->p_slptime;
}

static void
gsched_cfs_setup(gsd, p)
	struct gsched *gsd;
	struct proc *p;
{
	register struct gsched_cfs *cfs = gsched_cfs(gsd);

	if (cfs == NULL) {
		MALLOC(cfs, struct gsched_cfs *, sizeof(struct gsched_cfs *), M_GSCHED, M_WAITOK);
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

/* set estcpu */
void
gsched_estcpu(estcpu1, estcpu2)
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
gsched_cpticks(cpticks1, cpticks2)
	int cpticks1, cpticks2;
{
	if(cpticks1 != cpticks2) {
		cpticks1 = cpticks2;
	}
}

/* compare cpu ticks (deadline) of cur proc and the next proc in run-queue */
int
gsched_compare(a1, a2)
   const void *a1, *a2;
{
    struct proc *p1, *p2;
    p1 = (struct proc *)a1;
    p2 = (struct proc *)a2;
    
    if(p1->p_cpticks < p2->p_cpticks) {
        return (-1);
    } else if(p1->p_cpticks > p2->p_cpticks) {
        return (1);
    }
    return (0);
}

/* Initial sort of run-queue (lowest first): using gsched_compare; see above */
void
gsched_sort(cur)
    struct proc *cur;  
{
    struct proc *nxt;
    nxt = LIST_NEXT(cur, p_list); 
    LIST_FOREACH(cur, &allproc, p_list) {
        if(gsched_compare(cur, nxt) > 0) {
            qsort(cur, NQS, sizeof(struct proc), gsched_compare);
        }
    }
}
