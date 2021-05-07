/*-
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 * Copyright (c) 1994 John S. Dyson
 * All rights reserved.
 * Copyright (c) 1994 David Greenman
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
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
 *	from:	@(#)pmap.c	7.7 (Berkeley)	5/12/91
 */
/*-
 * Copyright (c) 2003 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Jake Burkholder,
 * Safeport Network Services, and Network Associates Laboratories, the
 * Security Research Division of Network Associates, Inc. under
 * DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA
 * CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

void
invltlb_glob(void)
{
	invltlb();
}

#ifdef I386_CPU
/*
 * i386 only has "invalidate everything" and no SMP to worry about.
 */
static __inline void
pmap_invalidate_page(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	if (pmap == kernel_pmap || pmap->pm_active) {
		invltlb();
	}
}

static __inline void
pmap_invalidate_range(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	if (pmap == kernel_pmap || pmap->pm_active) {
		invltlb();
	}
}

static __inline void
pmap_invalidate_all(pmap)
	pmap_t pmap;
{
	if (pmap == kernel_pmap || pmap->pm_active) {
		invltlb();
	}
}

#else /* !I386_CPU */
#ifdef SMP
/*
 * For SMP, these functions have to use the IPI mechanism for coherence.
 */
__inline void
pmap_invalidate_page(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	u_int cpumask;
	u_int other_cpus;

	if (smp_started) {
		if (!(read_eflags() & PSL_I))
			panic("%s: interrupts disabled", __func__);
		mtx_lock_spin(&smp_tlb_mtx);
	} else
		critical_enter();
	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap || pmap->pm_active == all_cpus) {
		invlpg(va);
		smp_invlpg(va);
	} else {
		cpumask = PERCPU_GET(cpumask);
		other_cpus = PERCPU_GET(other_cpus);
		if (pmap->pm_active & cpumask)
			invlpg(va);
		if (pmap->pm_active & other_cpus)
			smp_masked_invlpg(pmap->pm_active & other_cpus, va);
	}
	if (smp_started) {
		mtx_unlock_spin(&smp_tlb_mtx);
	} else {
		critical_exit();
	}
}

__inline void
pmap_invalidate_range(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	u_int cpumask;
	u_int other_cpus;
	vm_offset_t addr;

	if (smp_started) {
		if (!(read_eflags() & PSL_I))
			panic("%s: interrupts disabled", __func__);
		mtx_lock_spin(&smp_tlb_mtx);
	} else
		critical_enter();
	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap || pmap->pm_active == all_cpus) {
		for (addr = sva; addr < eva; addr += PAGE_SIZE)
			invlpg(addr);
		smp_invlpg_range(sva, eva);
	} else {
		cpumask = PERCPU_GET(cpumask);
		other_cpus = PERCPU_GET(other_cpus);
		if (pmap->pm_active & cpumask)
			for (addr = sva; addr < eva; addr += PAGE_SIZE)
				invlpg(addr);
		if (pmap->pm_active & other_cpus)
			smp_masked_invlpg_range(pmap->pm_active & other_cpus, sva, eva);
	}
	if (smp_started)
		mtx_unlock_spin(&smp_tlb_mtx);
	else
		critical_exit();
}

__inline void
pmap_invalidate_all(pmap)
	pmap_t pmap;
{
	u_int cpumask;
	u_int other_cpus;

	if (smp_started) {
		if (!(read_eflags() & PSL_I))
			panic("%s: interrupts disabled", __func__);
		mtx_lock_spin(&smp_tlb_mtx);
	} else
		critical_enter();
	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap || pmap->pm_active == all_cpus) {
		invltlb();
		smp_invltlb();
	} else {
		cpumask = PCPU_GET(cpumask);
		other_cpus = PCPU_GET(other_cpus);
		if (pmap->pm_active & cpumask)
			invltlb();
		if (pmap->pm_active & other_cpus)
			smp_masked_invltlb(pmap->pm_active & other_cpus);
	}
	if (smp_started)
		mtx_unlock_spin(&smp_tlb_mtx);
	else
		critical_exit();
}
#else /* !SMP */

/*
 * Normal, non-SMP, 486+ invalidation functions.
 * We inline these within pmap.c for speed.
 */
__inline void
pmap_invalidate_page(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	if (pmap == kernel_pmap || pmap->pm_active)
		invlpg(va);
}

__inline void
pmap_invalidate_range(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	vm_offset_t addr;

	if (pmap == kernel_pmap || pmap->pm_active) {
		for (addr = sva; addr < eva; addr += PAGE_SIZE) {
			invlpg(addr);
		}
	}
}

__inline void
pmap_invalidate_all(pmap)
	pmap_t pmap;
{
	if (pmap == kernel_pmap || pmap->pm_active) {
		invltlb();
	}
}
#endif /* !SMP */
#endif /* !I386_CPU */
