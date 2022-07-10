/*	$NetBSD: pmap.c,v 1.171.2.1.2.2 2005/08/24 04:08:09 riz Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/msgbuf.h>
#include <sys/memrange.h>
#include <sys/sysctl.h>
#include <sys/cputopo.h>
#include <sys/queue.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_param.h>

#include <machine/atomic.h>
#include <machine/cpu.h>
#include <machine/cputypes.h>
#include <machine/cpuvar.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <machine/specialreg.h>
#include <machine/vmparam.h>
#ifdef SMP
#include <machine/smp.h>
#endif
#include <machine/pmap_reg.h>
#include <machine/pmap_tlb.h>

/*
 * pmap_pg_g: if our processor supports PG_G in the PTE then we
 * set pmap_pg_g to PG_G (otherwise it is zero).
 */

int pmap_pg_g = 0;

/******************** TLB shootdown code ********************/

/*
 * TLB Shootdown:
 *
 * When a mapping is changed in a pmap, the TLB entry corresponding to
 * the virtual address must be invalidated on all processors.  In order
 * to accomplish this on systems with multiple processors, messages are
 * sent from the processor which performs the mapping change to all
 * processors on which the pmap is active.  For other processors, the
 * ASN generation numbers for that processor is invalidated, so that
 * the next time the pmap is activated on that processor, a new ASN
 * will be allocated (which implicitly invalidates all TLB entries).
 *
 * Shootdown job queue entries are allocated using a simple special-
 * purpose allocator for speed.
 */

/*
 * Initialize the TLB shootdown queues.
 */
void
pmap_tlb_init(void)
{
	int i;
	simple_lock_init(&pmap_tlb_shootdown_job_lock, "pmap_tlb_shootdown_job_lock");
	for (i = 0; i < NCPUS; i++) {
		TAILQ_INIT(&pmap_tlb_shootdown_q[i].pq_head);
		simple_lock_init(&pmap_tlb_shootdown_q[i].pq_slock, "pmap_tlb_shootdown_lock");
	}

	/*
	 * ensure the TLB is sync'd with reality by flushing it...
	 */
	tlbflush();
}

void
pmap_tlb_shootnow(pmap, cpumask)
	pmap_t 	pmap;
	int32_t cpumask;
{
	struct cpu_info *self;
#ifdef SMP
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	int s;
#endif
#ifdef DIAGNOSTIC
	int count = 0;
#endif

	if (cpumask == 0) {
		return;
	}

	self = curcpu();
#ifdef SMP
	s = splipi();
	self->cpu_tlb_ipi_mask = cpumask;
#endif
	pmap_do_tlb_shootdown(pmap, self);

#ifdef SMP
	splx(s);

	/*
	 * Send the TLB IPI to other CPUs pending shootdowns.
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == self)
			continue;
		if (cpumask & (1U << ci->cpu_cpuid)) {
			if (i386_send_ipi(ci, I386_IPI_TLB) != 0) {
				i386_atomic_clearbits_l(&self->cpu_tlb_ipi_mask, (1U << ci->cpu_cpuid));
			}
		}
	}
	while (self->cpu_tlb_ipi_mask != 0) {
#ifdef DIAGNOSTIC
		if (count++ > 10000000) {
			panic("TLB IPI rendezvous failed (mask %x)", self->cpu_tlb_ipi_mask);
		}
#endif
		ia32_pause();
	}
#endif
}

/*
 * Returns a PTE from address.
 */
pt_entry_t
pmap_tlb_pte(sva, eva)
	vm_offset_t sva, eva;
{
	pt_entry_t 	pte;
	vm_offset_t addr;

	if((sva == 0 && eva == 0) || (sva != 0 && eva == 0)) {
		addr = sva;
		if(vtopte(addr) != 0) {
			pte = vtopte(addr);
			return (pte);
		}
	} else {
		for (addr = sva; addr < eva; addr += PAGE_SIZE) {
			if (vtopte(addr) != 0) {
				pte = vtopte(addr);
				return (pte);
			}
		}
	}
	return (0);
}

/*
 * pmap_tlb_shootdown:
 *
 *	Cause the TLB entry for pmap/va to be shot down.
 */
void
pmap_tlb_shootdown(pmap, addr1, addr2, pte, mask)
	pmap_t 		pmap;
	vm_offset_t addr1, addr2;
	pt_entry_t 	pte;
	int32_t 	*mask;
{
	struct cpu_info *ci, *self;
	struct pmap_tlb_shootdown_q *pq;
	struct pmap_tlb_shootdown_job *pj;
	CPU_INFO_ITERATOR cii;
	int s;

	if (pmap_initialized == FALSE /*|| cpus_attached == 0*/) {
		invlpg(addr1);
		return;
	}

	self = curcpu();

	s = splipi();

#if 0
	printf("dshootdown %lx\n", addr1);
#endif

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (pmap == kernel_pmap() || pmap->pm_active) {
			continue;
		}
		if (ci != self && !(ci->cpu_flags & CPUF_RUNNING)) {
			continue;
		}

		pq = &pmap_tlb_shootdown_q[ci->cpu_cpuid];
		simple_lock(&pq->pq_slock);

		/*
		 * If there's a global flush already queued, or a
		 * non-global flush, and this pte doesn't have the G
		 * bit set, don't bother.
		 */
		if (pq->pq_flushg > 0 ||
				(pq->pq_flushu > 0 && (pte & pmap_pg_g) == 0)) {
			simple_unlock(&pq->pq_slock);
			continue;
		}

#ifdef I386_CPU
		if (cpu_class == CPUCLASS_386) {
			pq->pq_flushu++;
			*mask |= 1U << ci->ci_cpuid;
			simple_unlock(&pq->pq_slock);
			continue;
		}
#endif
		pj = pmap_tlb_shootdown_job_get(pq);
		pq->pq_pte |= pte;
		if (pj == NULL) {
			/*
			* Couldn't allocate a job entry.
 			* Kill it now for this CPU, unless the failure
 			* was due to too many pending flushes; otherwise,
 			* tell other cpus to kill everything..
 			*/
			if (ci == self && pq->pq_count < PMAP_TLB_MAXJOBS) {
				invlpg(addr1);
				simple_unlock(&pq->pq_slock);
				continue;
			} else {
				if (pq->pq_pte & pmap_pg_g) {
					pq->pq_flushg++;
				} else {
					pq->pq_flushu++;
				}
				/*
				 * Since we've nailed the whole thing,
				 * drain the job entries pending for that
				 * processor.
				 */
				pmap_tlb_shootdown_q_drain(pq);
				*mask |= 1U << ci->cpu_cpuid;
			}
		} else {
			pj->pj_pmap = pmap;
			pj->pj_sva = addr1;
			pj->pj_eva = addr2;
			pj->pj_pte = pte;
			TAILQ_INSERT_TAIL(&pq->pq_head, pj, pj_list);
			*mask |= 1U << ci->cpu_cpuid;
		}
		simple_unlock(&pq->pq_slock);
	}
	splx(s);
}

/*
 * pmap_do_tlb_shootdown:
 *
 *	Process pending TLB shootdown operations for this processor.
 */
void
pmap_do_tlb_shootdown(pmap, self)
	pmap_t pmap;
	struct cpu_info *self;
{
	u_long cpu_id = self->cpu_cpuid;
	struct pmap_tlb_shootdown_q *pq = &pmap_tlb_shootdown_q[cpu_id];
	struct pmap_tlb_shootdown_job *pj;
	int s;
#ifdef SMP
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
#endif

	KASSERT(self == curcpu());

	s = splipi();

	simple_lock(&pq->pq_slock);
	if (pq->pq_flush) {
		tlbflush();
		pq->pq_flushg = 0;
		pq->pq_flushu = 0;
		pmap_tlb_shootdown_q_drain(pq);
	} else {
		if (pq->pq_flushu) {
			tlbflush();
		}
		while ((pj = TAILQ_FIRST(&pq->pq_head)) != NULL) {
			TAILQ_REMOVE(&pq->pq_head, pj, pj_list);
			if ((pj->pj_pte & pmap_pg_g) || pj->pj_pmap == kernel_pmap() || pj->pj_pmap == pmap) {
				invlpg(pj->pj_va);
			}
			pmap_tlb_shootdown_job_put(pq, pj);
		}
		pq->pq_flushu = 0;
		pq->pq_pte = 0;
	}
#ifdef SMP
	for (CPU_INFO_FOREACH(cii, ci)) {
		i386_atomic_clearbits_l(&ci->cpu_tlb_ipi_mask, (1U << cpu_id));
	}
#endif
	simple_unlock(&pq->pq_slock);

	splx(s);
}

/*
 * pmap_tlb_shootdown_q_drain:
 *
 *	Drain a processor's TLB shootdown queue.  We do not perform
 *	the shootdown operations.  This is merely a convenience
 *	function.
 *
 *	Note: We expect the queue to be locked.
 */
void
pmap_tlb_shootdown_q_drain(pq)
	struct pmap_tlb_shootdown_q *pq;
{
	struct pmap_tlb_shootdown_job *pj;

	while ((pj = TAILQ_FIRST(&pq->pq_head)) != NULL) {
		TAILQ_REMOVE(&pq->pq_head, pj, pj_list);
		pmap_tlb_shootdown_job_put(pq, pj);
	}
	pq->pq_pte = 0;
}

/*
 * pmap_tlb_shootdown_job_get:
 *
 *	Get a TLB shootdown job queue entry.  This places a limit on
 *	the number of outstanding jobs a processor may have.
 *
 *	Note: We expect the queue to be locked.
 */
struct pmap_tlb_shootdown_job *
pmap_tlb_shootdown_job_get(pq)
	struct pmap_tlb_shootdown_q *pq;
{
	struct pmap_tlb_shootdown_job *pj;

	if (pq->pq_count >= PMAP_TLB_MAXJOBS) {
		return (NULL);
	}

	simple_lock(&pmap_tlb_shootdown_job_lock);
	if (pj_free == NULL) {
		simple_unlock(&pmap_tlb_shootdown_job_lock);
		return (NULL);
	}
	pj = &pj_free->pja_job;
	pj_free = (union pmap_tlb_shootdown_job_al*) pj_free->pja_job.pj_nextfree;
	simple_unlock(&pmap_tlb_shootdown_job_lock);

	pq->pq_count++;
	return (pj);
}

/*
 * pmap_tlb_shootdown_job_put:
 *
 *	Put a TLB shootdown job queue entry onto the free list.
 *
 *	Note: We expect the queue to be locked.
 */
void
pmap_tlb_shootdown_job_put(pq, pj)
	struct pmap_tlb_shootdown_q 	*pq;
	struct pmap_tlb_shootdown_job 	*pj;
{
#ifdef DIAGNOSTIC
	if (pq->pq_count == 0) {
		panic("pmap_tlb_shootdown_job_put: queue length inconsistency");
	}
#endif
	simple_lock(&pmap_tlb_shootdown_job_lock);
	pj->pj_nextfree = &pj_free->pja_job;
	pj_free = (union pmap_tlb_shootdown_job_al*) pj;
	simple_unlock(&pmap_tlb_shootdown_job_lock);

	pq->pq_count--;
}
