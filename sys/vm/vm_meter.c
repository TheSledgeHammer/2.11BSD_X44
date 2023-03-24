/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_meter.c	8.7 (Berkeley) 5/10/95
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/map.h>
#include <sys/tree.h>

#include <vm/include/vm.h>

#define	MINFINITY	-32767			/* minus infinity */

struct vmrate	cnt, rate;
struct vmsum	sum;
struct vmtotal 	total;
struct loadavg 	averunnable;		/* load average, of runnable procs */

int	maxslp = 	MAXSLP;
int	saferss = 	SAFERSS;

size_t freemem;
int	avefree;
int	avefree30;
int nrun;
int	avenrun[3];

void
vmmeter(void)
{
	register u_int *cp, *rp;
	register long *sp;

	ave(avefree, freemem, 5);
	ave(avefree30, freemem, 30);
	cp = &cnt.v_first;
	rp = &rate.v_first;
	sp = &sum.v_first;

	while (cp <= &cnt.v_last) {
		ave(*rp, *cp, 5);
		*sp += *cp;
		*cp = 0;
		rp++, cp++, sp++;
	}
	if (time.tv_sec % 5 == 0) {
		loadav(&averunnable);
		rate.v_swpin = cnt.v_swpin;
		sum.v_swpin += cnt.v_swpin;
		cnt.v_swpin = 0;
		rate.v_swpout = cnt.v_swpout;
		sum.v_swpout += cnt.v_swpout;
		cnt.v_swpout = 0;
	}
	if (proc0.p_slptime > maxslp/2) {
		wakeup((caddr_t)&proc0);
	}
}

/*
 * Constants for averages over 1, 5, and 15 minutes
 * when sampling at 5 second intervals.
 */
fixpt_t	cexp[3] = {
	0.9200444146293232 * FSCALE,	/* exp(-1/12) */
	0.9834714538216174 * FSCALE,	/* exp(-1/60) */
	0.9944598480048967 * FSCALE,	/* exp(-1/180) */
};

/*
 * Compute a tenex style load average of a quantity on
 * 1, 5 and 15 minute intervals.
 */
void
loadav(avg)
	register struct loadavg *avg;
{
	register int i;
	register struct proc *p;

	for (nrun = 0, p = LIST_FIRST(&allproc); p != NULL; p = LIST_NEXT(p, p_list)) {
		switch (p->p_stat) {
		case SSLEEP:
			if (p->p_pri > PZERO || p->p_slptime != 0)
				continue;
			/* fall through */
			break;
		case SRUN:
		case SIDL:
			nrun++;
		}
	}
	for (i = 0; i < 3; i++) {
		avg->ldavg[i] = (cexp[i] * avg->ldavg[i] +
			nrun * FSCALE * (FSCALE - cexp[i])) >> FSHIFT;
	}
}

/*
 * Bit of a hack.  2.11 currently uses 'short avenrun[3]' and a fixed scale
 * of 256.  In order not to break all the applications which nlist() for
 * 'avenrun' we build a local 'averunnable' structure here to return to the
 * user.  Eventually (after all applications which look up the load average
 * the old way) have been converted we can change things.
 *
 * We do not call vmtotal(), that could get rather expensive, rather we rely
 * on the 5 second update.
 *
 * The coremap and swapmap cases are 2.11BSD extensions.
*/
/*
 * Attributes associated with virtual memory.
 */
int
vm_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	struct vmtotal vmtotals;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case VM_LOADAVG:
		averunnable.fscale = FSCALE;
		averunnable.ldavg[0] = avenrun[0];
		averunnable.ldavg[1] = avenrun[1];
		averunnable.ldavg[2] = avenrun[2];
		return (sysctl_rdstruct(oldp, oldlenp, newp, &averunnable, sizeof(averunnable)));
	case VM_METER:
		vmtotal(&vmtotals);
		return (sysctl_rdstruct(oldp, oldlenp, newp, &vmtotals, sizeof(vmtotals)));
	case VM_TEXT:
		return (sysctl_text(oldp, oldlenp));
	case VM_SWAPMAP:
		if (oldp == NULL) {
			*oldlenp = (char *)swapmap[0].m_limit - (char *)swapmap[0].m_map;
			return(0);
		}
		return (sysctl_rdstruct(oldp, oldlenp, newp, swapmap, (int)swapmap[0].m_limit - (int)swapmap[0].m_map));
	case VM_COREMAP:
		if (oldp == NULL) {
			*oldlenp = (char*) coremap[0].m_limit - (char*) coremap[0].m_map;
			return (0);
		}
		return (sysctl_rdstruct(oldp, oldlenp, newp, coremap, (int) coremap[0].m_limit - (int) coremap[0].m_map));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Calculate the current state of the system.
 * Done on demand from getkerninfo().
 */
void
vmtotal(totalp)
	register struct vmtotal *totalp;
{
	register struct proc *p;
	register vm_map_entry_t	entry;
	register vm_object_t object;
	register vm_map_t map;
	int paging;

	totalp->t_vm = 0;
	totalp->t_avm = 0;
	totalp->t_rm = 0;
	totalp->t_arm = 0;
	totalp->t_rq = 0;
	totalp->t_dw = 0;
	totalp->t_sl = 0;
	totalp->t_sw = 0;
	bzero(totalp, sizeof *totalp);

	/*
	 * Mark all objects as inactive.
	 */
	vm_object_mark_inactive(object);

	/*
	 * Calculate process statistics.
	 */
	LIST_FOREACH(p, &allproc, p_list) {
		if (p->p_flag & P_SYSTEM)
			continue;
		if (p->p_stat) {
			totalp->t_vm += p->p_dsize + p->p_ssize + UPAGES;
			if (p->p_flag & P_SLOAD)
				totalp->t_rm += p->p_dsize + p->p_ssize + UPAGES;
		}
		switch (p->p_stat) {
		case 0:
			continue;

		case SSLEEP:
			if (p->p_slptime >= maxslp)
				continue;
			break;
		case SSTOP:
			if (!(p->p_flag & P_SINTR) && p->p_stat == SSLEEP) {
				nrun++;
			}
			if (p->p_flag & (P_INMEM | P_SLOAD)) {
				if (p->p_pri <= PZERO || !(p->p_flag & P_SINTR))
					totalp->t_dw++;
				else if (p->p_slptime < maxslp)
					totalp->t_sl++;
			} else if (p->p_slptime < maxslp)
				totalp->t_sw++;
			if (p->p_slptime >= maxslp)
				goto active;
			break;

		case SRUN:
		case SIDL:
			nrun++;
			if (p->p_flag & (P_INMEM | P_SLOAD))
				totalp->t_rq++;
			else
				totalp->t_sw++;
active:
			totalp->t_avm += p->p_dsize + p->p_ssize + UPAGES;
			if (p->p_flag & P_SLOAD)
				totalp->t_arm += p->p_dsize + p->p_ssize + UPAGES;
			if (p->p_stat == SIDL)
				continue;
			break;
		}

		/*
		 * Note active objects.
		 *
		 * XXX don't count shadow objects with no resident pages.
		 * This eliminates the forced shadows caused by MAP_PRIVATE.
		 * Right now we require that such an object completely shadow
		 * the original, to catch just those cases.
		 */

		paging = 0;
		for (map = &p->p_vmspace->vm_map, entry =
				CIRCLEQ_FIRST(&map->cl_header)->cl_entry.cqe_next;
				entry != CIRCLEQ_FIRST(&map->cl_header);
				entry = CIRCLEQ_NEXT(entry, cl_entry)) {
			if (entry->is_a_map || entry->is_sub_map
					|| (object = entry->object.vm_object) == NULL) {
				continue;
			}
			while (object->shadow && object->resident_page_count == 0
					&& object->shadow_offset == 0
					&& object->size == object->shadow->size) {
				object = object->shadow;
				object->flags |= OBJ_ACTIVE;
				paging |= object->paging_in_progress;
			}
			if (paging) {
				totalp->t_pw++;
			}
		}
	}

	/*
	 * Calculate object memory usage statistics.
	 */
	vm_object_mem_stats(totalp, object);

	totalp->t_vm += totalp->t_vmtxt;
	totalp->t_avm += totalp->t_avmtxt;
	totalp->t_rm += totalp->t_rmtxt;
	totalp->t_arm += totalp->t_armtxt;
	totalp->t_free = (long)cnt.v_page_free_count;
	avefree = cnt.v_page_free_count;
}
