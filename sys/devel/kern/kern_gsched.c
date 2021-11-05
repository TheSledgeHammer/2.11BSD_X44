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

/* The Global Scheduler Interface:
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

#include "devel/sys/gsched.h"
#include "devel/sys/gsched_cfs.h"
#include "devel/sys/gsched_edf.h"
#include "devel/sys/malloctypes.h"

static struct gsched *gsched_setup(struct proc *);
static void	gsched_edf_setup(struct gsched *, struct proc *);
static void	gsched_cfs_setup(struct gsched *, struct proc *);
static int gsched_compare(struct proc *, struct proc *);

void
gsched_init(p)
	struct proc *p;
{
	struct gsched *gsd = gsched_setup(p);
	gsched_edf_setup(gsd, p);
	gsched_cfs_setup(gsd, p);
}

static struct gsched *
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
gsched_timediff(timo, estcpu)
	u_char timo;
	u_int estcpu;
{
	u_char diff;
	if (timo > estcpu) {
		diff = timo - estcpu;
	} else {
		diff = estcpu - timo;
	}
	return (diff);
}

/* compare cpu ticks (deadline) of cur proc and the next proc in run-queue */
static int
gsched_compare(p1, p2)
    struct proc *p1, *p2;
{
    if(p1->p_cpticks < p2->p_cpticks) {
        return (-1);
    } else if(p1->p_cpticks > p2->p_cpticks) {
        return (1);
    }
    return (0);
}

/* Initial sort of run-queue (lowest first): using gsched_compare; see above */
void
gsched_sort(cur, nxt)
    struct proc *cur;
    struct proc *nxt;
{
    struct proc *tmp;

    if(gsched_compare(cur, nxt) > 0) {
        tmp = cur;
        cur = nxt;
        nxt = tmp;

        if(cur->p_nxt != nxt) {
            cur->p_nxt = nxt;
        }
    }
}
