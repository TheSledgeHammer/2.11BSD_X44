/*
 * Copyright (c) 1996, by Steve Passe
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 * $FreeBSD$
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
#include <machine/pmap_base.h>
#include <machine/pmap_tlb.h>

vm_offset_t 		smp_tlb_addr1, smp_tlb_addr2;
volatile int 		smp_tlb_wait;

static __inline u_int32_t
popcnt(u_int32_t m)
{
	m = (m & 0x55555555) + ((m & 0xaaaaaaaa) >> 1);
	m = (m & 0x33333333) + ((m & 0xcccccccc) >> 2);
	m = (m & 0x0f0f0f0f) + ((m & 0xf0f0f0f0) >> 4);
	m = (m & 0x00ff00ff) + ((m & 0xff00ff00) >> 8);
	m = (m & 0x0000ffff) + ((m & 0xffff0000) >> 16);
	return (m);
}

static void
smp_targeted_tlb_shootdown(u_int mask, u_int vector, pmap_t pmap, vm_offset_t addr1, vm_offset_t addr2)
{
	int ncpu, othercpus;
	register_t eflags;

	if (mask == (u_int)-1) {
		ncpu = othercpus;
		if (ncpu < 1) {
			return;
		}
	} else {
		mask &= ~(1 << &cpuid_to_percpu[ncpu]);
		if (mask == 0) {
			return;
		}
		ncpu = popcnt(mask);
		if (ncpu > othercpus) {
			printf("SMP: tlb shootdown to %d other cpus (only have %d)\n", ncpu, othercpus);
			ncpu = othercpus;
		}
		if (ncpu < 1) {
			return;
		}
	}

	eflags = read_eflags();
	if ((eflags & PSL_I) == 0) {
		panic("absolutely cannot call smp_targeted_ipi_shootdown with interrupts already disabled");
	}

	smp_tlb_addr1 = addr1;
	smp_tlb_addr2 = addr2;
	atomic_store_relaxed(&smp_tlb_wait, 0);

	if (mask == (u_int)-1) {
		i386_broadcast_ipi(vector);
	} else {
		i386_multicast_ipi(mask, vector);
	}
	while (smp_tlb_wait < ncpu) {
		ncpu--;
		pmap_tlb_shootdown(pmap, addr1, addr2, mask);
		pmap_tlb_shootnow(pmap, mask);
	}
	return;
}

__inline void
smp_masked_invltlb(u_int mask, pmap_t pmap)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLTLB, pmap, 0, 0);
	}
}

__inline void
smp_masked_invlpg(u_int mask, vm_offset_t addr, pmap_t pmap)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLPG, pmap, addr, 0);
	}
}

__inline void
smp_masked_invlpg_range(u_int mask, vm_offset_t addr1, vm_offset_t addr2, pmap_t pmap)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLRNG, pmap, addr1, addr2);
	}
}

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

	if (smp_started) {
		if (!(read_eflags() & PSL_I)) {
			panic("%s: interrupts disabled", __func__);
		}
	}

	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap() || pmap->pm_active == all_cpus) {
		invlpg(va);
	} else {
		cpumask = __PERCPU_GET(cpumask);
		if (pmap->pm_active & cpumask) {
			invlpg(va);
		} else {
			smp_masked_invlpg(cpumask, va, pmap);
		}
	}
}

__inline void
pmap_invalidate_range(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	u_int cpumask;
	vm_offset_t addr;

	if (smp_started) {
		if (!(read_eflags() & PSL_I)) {
			panic("%s: interrupts disabled", __func__);
		}
	}

	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap() || pmap->pm_active == all_cpus) {
		for (addr = sva; addr < eva; addr += PAGE_SIZE) {
			invlpg(addr);
		}
	} else {
		cpumask = __PERCPU_GET(cpumask);
		if (pmap->pm_active & cpumask) {
			for (addr = sva; addr < eva; addr += PAGE_SIZE) {
				invlpg(addr);
			}
		}
		smp_masked_invlpg_range(cpumask, sva, eva, pmap);
	}
}

__inline void
pmap_invalidate_all(pmap)
	pmap_t pmap;
{
	u_int cpumask;

	if (smp_started) {
		if (!(read_eflags() & PSL_I)) {
			panic("%s: interrupts disabled", __func__);
		}
	}

	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap() || pmap->pm_active == all_cpus) {
		invltlb();
	} else {
		cpumask = __PERCPU_GET(cpumask);
		if (pmap->pm_active & cpumask) {
			invltlb();
		}
		smp_masked_invltlb(cpumask, pmap);
	}
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
	if (pmap == kernel_pmap() || pmap->pm_active) {
		invlpg(va);
	}
}

__inline void
pmap_invalidate_range(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	vm_offset_t addr;

	if (pmap == kernel_pmap() || pmap->pm_active) {
		for (addr = sva; addr < eva; addr += PAGE_SIZE) {
			invlpg(addr);
		}
	}
}

__inline void
pmap_invalidate_all(pmap)
	pmap_t pmap;
{
	if (pmap == kernel_pmap() || pmap->pm_active) {
		invltlb();
	}
}
#endif /* !SMP */
