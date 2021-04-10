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

/*
 * Local APIC support on Pentium and later processors.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <vm/include/vm_extern.h>

#include <arch/i386/isa/isa_machdep.h> 			/* XXX intrhand */

#include <devel/arch/i386/include/apicreg.h>
#include <devel/arch/i386/include/apicvar.h>
#include <devel/arch/i386/include/cpu.h>
#include <devel/arch/i386/include/pic.h>

#include <arch/i386/include/intr.h>
#include <arch/i386/include/pio.h>
#include <arch/i386/include/pmap.h>
#include <arch/i386/include/pte.h>

#include <arch/i386/include/specialreg.h>

extern volatile vaddr_t local_apic_va;
//volatile char 			*lapic_map;

static void 	lapic_hwmask(struct ioapic_intsrc *, int);
static void 	lapic_hwunmask(struct ioapic_intsrc *, int);
static void 	lapic_setup(struct ioapic_intsrc *, struct cpu_info *, int, int, int);
void			lapic_map(caddr_t);

struct pic lapic_template = {
		.pic_type = PIC_LAPIC,
		.pic_hwmask = lapic_hwmask,
		.pic_hwunmask = lapic_hwunmask,
		.pic_addroute = lapic_setup,
		.pic_delroute = lapic_setup,
};

/* i82489_readreg */
static uint32_t
i82489_read32(int reg)
{
	uint32_t res;

	res = *((volatile uint32_t *)(local_apic_va + reg));
	return (res);
}

/* i82489_writereg */
static void
i82489_write32(int reg, uint32_t val)
{
	*((volatile uint32_t *)(local_apic_va + reg)) = val;
}

/* i82489_cpu_number */
static uint32_t
i82489_cpu_number(void)
{
	return (lapic_read32(LAPIC_ID) >> LAPIC_ID_SHIFT);
}

static uint32_t
x2apic_read32(u_int reg)
{
	return (rdmsr(MSR_APICBASE + (reg >> 4)));
}

static void
x2apic_write32(u_int reg, uint32_t val)
{
	mfence();
	wrmsr(MSR_APICBASE + (reg >> 4), val);
}

static void
x2apic_write64(u_int reg, uint64_t val)
{
	KDASSERT(reg == LAPIC_ICRLO);
	mfence();
	wrmsr(MSR_APICBASE + (reg >> 4), val);
}

static void
x2apic_write_icr(uint32_t hi, uint32_t lo)
{
	x2apic_write64(LAPIC_ICRLO, ((uint64_t) hi << 32) | lo);
}

static uint32_t
x2apic_cpu_number(void)
{
	return x2apic_readreg(LAPIC_ID);
}

uint32_t
lapic_read(u_int reg)
{
	if (x2apic_mode) {
		return x2apic_read32(reg);
	}
	return i82489_read32(reg);
}

void
lapic_write(u_int reg, uint32_t val)
{
	if (x2apic_mode) {
		x2apic_write32(reg, val);
	} else {
		i82489_write32(reg, val);
	}
}

void
lapic_write_tpri(uint32_t val)
{
	val &= LAPIC_TPRI_MASK;
#ifdef i386
	lapic_write(LAPIC_TPRI, val);
#else
	lcr8(val >> 4);
#endif
}

uint32_t
lapic_cpu_number(void)
{
	if (x2apic_mode) {
		return (x2apic_cpu_number());
	}
	return (i82489_cpu_number());
}

static void
lapic_enable_x2apic(void)
{
	uint64_t apicbase;

	apicbase = rdmsr(MSR_APICBASE);
	apicbase |= APICBASE_X2APIC | APICBASE_ENABLED;
	wrmsr(MSR_APICBASE, apicbase);
}

boolean_t
lapic_is_x2apic(void)
{
	uint64_t apicbase;

	apicbase = rdmsr(MSR_APICBASE);
	return ((apicbase & (APICBASE_X2APIC | APICBASE_ENABLED)) == (APICBASE_X2APIC | APICBASE_ENABLED));
}

void
lapic_map(caddr_t lapic_base)
{
	pt_entry_t *pte;
	vaddr_t va = (vaddr_t)&local_apic_va;

	intr_disable();

	pte = kvtopte(va);
	*pte = (lapic_base | PG_RW | PG_V | PG_N | PG_G | PG_NX | PG_W | PG_P | PG_NC_PCD); /* TODO: check what are valid pte's here */

	invlpg();
	enable_intr();
}

/*
 * Using 'pin numbers' as:
 * 0 - timer
 * 1 - unused
 * 2 - PCINT
 * 3 - LVINT0
 * 4 - LVINT1
 * 5 - LVERR
 */

static void
lapic_hwmask(struct ioapic_intsrc *intpin, int pin)
{
	int reg;
	u_int32_t val;

	reg = LAPIC_PIN_MASK(LAPIC_LVTT, pin);
	val = lapic_read(reg);
	val |= LAPIC_LVT_MASKED;
	lapic_write(reg, val);
}

static void
lapic_hwunmask(struct ioapic_intsrc *intpin, int pin)
{
	int reg;
	u_int32_t val;

	reg = LAPIC_PIN_MASK(LAPIC_LVTT, pin);
	val = lapic_read(reg);
	val &= ~LAPIC_LVT_MASKED;
	lapic_write(reg, val);
}

static void
lapic_setup(struct ioapic_intsrc *intpin, struct cpu_info *ci, int pin, int idtvec, int type)
{

}
