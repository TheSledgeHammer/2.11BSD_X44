/*-
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
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/memrange.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

#include <vm/include/vm.h>
#include <vm/include/vm_param.h>
#include <vm/include/pmap.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_extern.h>

#include <arch/i386/include/cputypes.h>
#include <arch/i386/include/pcb.h>
#include <arch/i386/include/psl.h>
#include <arch/i386/include/pte.h>
#include <arch/i386/include/param.h>
#include <arch/i386/include/specialreg.h>
#include <arch/i386/include/vmparam.h>
#include <arch/i386/include/vm86.h>

#include <devel/arch/i386/include/smp.h>
#include <arch/i386/include/cpu.h>
#include <devel/arch/i386/include/percpu.h>
#include "../../../../sys/percpu.h"

/* TODO:
 * modify: smp_tlb_shootdown & smp_targeted_tlb_shootdown
 *
 * config:
 * options 	COUNT_XINVLTLB_HITS	# Counters for TLB events
 */

/* FreeBSD 5.1 SMP with a few updates */

/*
 * Flush the TLB on other CPU's
 */
/* Variables needed for SMP tlb shootdown. */

#ifdef COUNT_XINVLTLB_HITS
u_int 				xhits_gbl[NCPUS];
u_int 				xhits_pg[NCPUS];
u_int 				xhits_rng[NCPUS];

u_int 				ipi_global;
u_int 				ipi_page;
u_int 				ipi_range;
u_int 				ipi_range_size;
u_int 				ipi_masked_global;
u_int 				ipi_masked_page;
u_int 				ipi_masked_range;
u_int 				ipi_masked_range_size;

vm_offset_t 		smp_tlb_addr1, smp_tlb_addr2;
pmap_t 				smp_tlb_pmap;
volatile u_int32_t 	smp_tlb_generation;
volatile int 		smp_tlb_wait;

static void
smp_tlb_shootdown(u_int vector, vm_offset_t addr1, vm_offset_t addr2)
{
	u_int ncpu;
	register_t eflags;

	ncpu = mp_ncpus - 1;	/* does not shootdown self */
	if (ncpu < 1) {
		return; 			/* no other cpus */
	}
	eflags = read_eflags();
	if ((eflags & PSL_I) == 0) {
		panic("absolutely cannot call smp_ipi_shootdown with interrupts already disabled");
	}
	simple_lock(&smp_tlb_lock);
	smp_tlb_addr1 = addr1;
	smp_tlb_addr2 = addr2;
	atomic_store_relaxed(&smp_tlb_wait, 0);
	i386_self_ipi(vector);
	while (smp_tlb_wait < ncpu) {
		ia32_pause();
	}
	simple_unlock(&smp_tlb_lock);
}

/*
 * This is about as magic as it gets.  fortune(1) has got similar code
 * for reversing bits in a word.  Who thinks up this stuff??
 *
 * Yes, it does appear to be consistently faster than:
 * while (i = ffs(m)) {
 *	m >>= i;
 *	bits++;
 * }
 * and
 * while (lsb = (m & -m)) {	// This is magic too
 * 	m &= ~lsb;		// or: m ^= lsb
 *	bits++;
 * }
 * Both of these latter forms do some very strange things on gcc-3.1 with
 * -mcpu=pentiumpro and/or -march=pentiumpro and/or -O or -O2.
 * There is probably an SSE or MMX popcnt instruction.
 *
 * I wonder if this should be in libkern?
 *
 * XXX Stop the presses!  Another one:
 * static __inline u_int32_t
 * popcnt1(u_int32_t v)
 * {
 *	v -= ((v >> 1) & 0x55555555);
 *	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
 *	v = (v + (v >> 4)) & 0x0F0F0F0F;
 *	return (v * 0x01010101) >> 24;
 * }
 * The downside is that it has a multiply.  With a pentium3 with
 * -mcpu=pentiumpro and -march=pentiumpro then gcc-3.1 will use
 * an imull, and in that case it is faster.  In most other cases
 * it appears slightly slower.
 *
 * Another variant (also from fortune):
 * #define BITCOUNT(x) (((BX_(x)+(BX_(x)>>4)) & 0x0F0F0F0F) % 255)
 * #define  BX_(x)     ((x) - (((x)>>1)&0x77777777)            \
 *                          - (((x)>>2)&0x33333333)            \
 *                          - (((x)>>3)&0x11111111))
 */
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
smp_targeted_tlb_shootdown(u_int mask, u_int vector, pmap_t pmap, vm_offset_t addr1, vm_offset_t addr2, smp_invl_cb_t curcpu_cb)
{
	volatile u_int32_t *p_cpudone;
	u_int32_t generation;
	int ncpu, othercpus;
	register_t eflags;

	othercpus = mp_ncpus - 1;
	if (mask == (u_int) -1) {
		ncpu = othercpus;
		if (ncpu < 1) {
			return;
		}
	} else {
		/* XXX there should be a percpu self mask */
		mask &= ~(1 << &cpuid_to_percpu[ncpu]);
		if (mask == 0) {
			return;
		}
		ncpu = popcnt(mask);
		if (ncpu > othercpus) {
			/* XXX this should be a panic offence */
			printf("SMP: tlb shootdown to %d other cpus (only have %d)\n", ncpu, othercpus);
			ncpu = othercpus;
		}
		/* XXX should be a panic, implied by mask == 0 above */
		if (ncpu < 1) {
			return;
		}
	}
	eflags = read_eflags();
	if ((eflags & PSL_I) == 0) {
		panic("absolutely cannotcall smp_targeted_ipi_shootdown with interrupts already disabled");
	}

	simple_lock(&smp_tlb_lock);
	curcpu_cb(pmap, addr1, addr2);

	smp_tlb_addr1 = addr1;
	smp_tlb_addr2 = addr2;

	atomic_store_relaxed(&smp_tlb_wait, 0);
	generation = ++smp_tlb_generation;
	curcpu_cb(pmap, addr1, addr2);

	while (smp_tlb_wait < ncpu) {
		ncpu--;
		p_cpudone = &cpuid_to_percpu[ncpu]->pc_smp_tlb_done;
		while (*p_cpudone != generation) {
			ia32_pause();
		}
	}
	simple_lock(&smp_tlb_lock);
	return;
}

void
smp_invltlb(void)
{
	if (smp_started) {
		smp_tlb_shootdown(IPI_INVLTLB, 0, 0);
#ifdef COUNT_XINVLTLB_HITS
		ipi_global++;
#endif
	}
}

void
smp_invlpg(vm_offset_t addr)
{
	if (smp_started) {
		smp_tlb_shootdown(IPI_INVLPG, addr, 0);
#ifdef COUNT_XINVLTLB_HITS
		ipi_page++;
#endif
	}
}

void
smp_invlpg_range(vm_offset_t addr1, vm_offset_t addr2)
{
	if (smp_started) {
		smp_tlb_shootdown(IPI_INVLRNG, addr1, addr2);
#ifdef COUNT_XINVLTLB_HITS
		ipi_range++;
		ipi_range_size += (addr2 - addr1) / PAGE_SIZE;
#endif
	}
}

void
smp_masked_invltlb(u_int mask, pmap_t pmap, smp_invl_cb_t curcpu_cb)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLTLB, pmap, 0, 0, curcpu_cb);
#ifdef COUNT_XINVLTLB_HITS
		ipi_masked_global++;
#endif
	}
}

void
smp_masked_invlpg(u_int mask, vm_offset_t addr, pmap_t pmap, smp_invl_cb_t curcpu_cb)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLPG, pmap, addr, 0, curcpu_cb);
#ifdef COUNT_XINVLTLB_HITS
		ipi_masked_page++;
#endif
	}
}

void
smp_masked_invlpg_range(u_int mask, vm_offset_t addr1, vm_offset_t addr2, pmap_t pmap, smp_invl_cb_t curcpu_cb)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLRNG, pmap, addr1, addr2, curcpu_cb);
#ifdef COUNT_XINVLTLB_HITS
		ipi_masked_range++;
		ipi_masked_range_size += (addr2 - addr1) / PAGE_SIZE;
#endif
	}
}

/*
 * Handlers for TLB related IPIs
 */
void
invltlb_handler(void)
{
	u_int32_t generation;

#ifdef COUNT_XINVLTLB_HITS
	xhits_gbl[PERCPU_GET(cpuid)]++;
#endif /* COUNT_XINVLTLB_HITS */

	generation = smp_tlb_generation;
	if (smp_tlb_pmap == kernel_pmap) {
		invltlb();
	}
	__PERCPU_SET(smp_tlb_done, generation);
}

void
invlpg_handler(void)
{
	u_int32_t generation;

#ifdef COUNT_XINVLTLB_HITS
	xhits_pg[PERCPU_GET(cpuid)]++;
#endif /* COUNT_XINVLTLB_HITS */

	generation = smp_tlb_generation;	/* Overlap with serialization */
	if (smp_tlb_pmap == kernel_pmap) {
		invlpg(smp_tlb_addr1);
	}
	__PERCPU_SET(smp_tlb_done, generation);
}

void
invlrng_handler(void)
{
	vm_offset_t addr, addr2;
	u_int32_t generation;

#ifdef COUNT_XINVLTLB_HITS
	xhits_rng[PERCPU_GET(cpuid)]++;
#endif /* COUNT_XINVLTLB_HITS */

	addr = smp_tlb_addr1;
	addr2 = smp_tlb_addr2;
	generation = smp_tlb_generation;	/* Overlap with serialization */
	if(smp_tlb_pmap == kernel_pmap) {
		do {
			invlpg(addr);
			addr += PAGE_SIZE;
		} while (addr < addr2);
	}

	__PERCPU_SET(smp_tlb_done, generation);
}
