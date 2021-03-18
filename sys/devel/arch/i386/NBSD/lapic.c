/*	$NetBSD: lapic.c,v 1.85 2020/10/27 08:57:11 ryo Exp $	*/

/*-
 * Copyright (c) 2000, 2008, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: lapic.c,v 1.85 2020/10/27 08:57:11 ryo Exp $"); */
#include <sys/user.h>
#include <arch/i386/include/specialreg.h>

#include <devel/arch/i386/NBSD/apicvar.h>
#include <devel/arch/i386/NBSD/i82489reg.h>
#include <devel/arch/i386/NBSD/i82489var.h>
#include <devel/arch/i386/NBSD/i82093var.h>

/* Referenced from vector.S */
void			lapic_clockintr(void *, struct intrframe *);

static void		lapic_delay(unsigned int);
static uint32_t lapic_gettick(void);
static void 	lapic_setup_bsp(paddr_t);
static void 	lapic_map(paddr_t);

static void 	lapic_hwmask(struct pic *, int);
static void 	lapic_hwunmask(struct pic *, int);
static void 	lapic_setup(struct pic *, struct cpu_info *, int, int, int);
/* Make it public to call via ddb */
void			lapic_dump(void);

struct pic local_pic = {
		.pic_name 				= "lapic",
		.pic_type 				= PIC_LAPIC,
		.pic_lock 				= __SIMPLELOCK_UNLOCKED,
		.pic_hwmask 			= lapic_hwmask,
		.pic_hwunmask 			= lapic_hwunmask,
		.pic_addroute 			= lapic_setup,
		.pic_delroute 			= lapic_setup,
		.pic_intr_get_devname 	= i386_intr_get_devname,
		.pic_intr_get_assigned 	= i386_intr_get_assigned,
		.pic_intr_get_count 	= i386_intr_get_count,
};

static int i82489_ipi(int vec, int target, int dl);
static int x2apic_ipi(int vec, int target, int dl);
int (*x86_ipi)(int, int, int) = i82489_ipi;

boolean_t x2apic_mode __read_mostly;
#ifdef LAPIC_ENABLE_X2APIC
bool x2apic_enable = true;
#else
bool x2apic_enable = false;
#endif

static boolean_t lapic_broken_periodic __read_mostly;

static uint32_t
i82489_readreg(u_int reg)
{
	return *((volatile uint32_t *)(local_apic_va + reg));
}

static void
i82489_writereg(u_int reg, uint32_t val)
{
	*((volatile uint32_t *)(local_apic_va + reg)) = val;
}

static uint32_t
i82489_cpu_number(void)
{
	return i82489_readreg(LAPIC_ID) >> LAPIC_ID_SHIFT;
}

static uint32_t
x2apic_readreg(u_int reg)
{
	return rdmsr(MSR_X2APIC_BASE + (reg >> 4));
}

static void
x2apic_writereg(u_int reg, uint32_t val)
{
	x86_mfence();
	wrmsr(MSR_X2APIC_BASE + (reg >> 4), val);
}

static void
x2apic_writereg64(u_int reg, uint64_t val)
{
	KDASSERT(reg == LAPIC_ICRLO);
	x86_mfence();
	wrmsr(MSR_X2APIC_BASE + (reg >> 4), val);
}

static void
x2apic_write_icr(uint32_t hi, uint32_t lo)
{
	x2apic_writereg64(LAPIC_ICRLO, ((uint64_t) hi << 32) | lo);
}

static uint32_t
x2apic_cpu_number(void)
{
	return x2apic_readreg(LAPIC_ID);
}

uint32_t
lapic_readreg(u_int reg)
{
	if (x2apic_mode)
		return x2apic_readreg(reg);
	return i82489_readreg(reg);
}

void
lapic_writereg(u_int reg, uint32_t val)
{
	if (x2apic_mode)
		x2apic_writereg(reg, val);
	else
		i82489_writereg(reg, val);
}

void
lapic_write_tpri(uint32_t val)
{
	val &= LAPIC_TPRI_MASK;
#ifdef i386
	lapic_writereg(LAPIC_TPRI, val);
#else
	lcr8(val >> 4);
#endif
}

uint32_t
lapic_cpu_number(void)
{
	if (x2apic_mode)
		return x2apic_cpu_number();
	return i82489_cpu_number();
}

static void
lapic_enable_x2apic(void)
{
	uint64_t apicbase;

	apicbase = rdmsr(MSR_APICBASE);
	if (!ISSET(apicbase, APICBASE_EN)) {
		apicbase |= APICBASE_EN;
		wrmsr(MSR_APICBASE, apicbase);
	}
	apicbase |= APICBASE_EXTD;
	wrmsr(MSR_APICBASE, apicbase);
}

boolean_t
lapic_is_x2apic(void)
{
	uint64_t msr;

	if (!ISSET(cpu_feature[0], CPUID_APIC) || rdmsr_safe(MSR_APICBASE, &msr) == EFAULT)
		return FALSE;
	return (msr & (APICBASE_EN | APICBASE_EXTD)) == (APICBASE_EN | APICBASE_EXTD);
}

void
lapic_map(caddr_t lapic_base)
{
	vaddr_t va = (vaddr_t)&local_apic;
	u_long s;
	int tpr;

	s = intr_disable();
	tpr = lapic_tpr;

	/*
	 * Map local apic.  If we have a local apic, it's safe to assume
	 * we're on a 486 or better and can use invlpg and non-cacheable PTEs
	 *
	 * Whap the PTE "by hand" rather than calling pmap_kenter_pa because
	 * the latter will attempt to invoke TLB shootdown code just as we
	 * might have changed the value of cpu_number()..
	 */

	pmap_pte_set(va, lapic_base, PG_RW | PG_V | PG_N);
	invlpg(va);

	pmap_enter_special(va, lapic_base, PROT_READ | PROT_WRITE, PG_N);
	DPRINTF("%s: entered lapic page va 0x%08lx pa 0x%08lx\n", __func__, va, lapic_base);

#ifdef MULTIPROCESSOR
	cpu_init_first();
#endif

	lapic_tpr = tpr;
	intr_restore(s);
}
