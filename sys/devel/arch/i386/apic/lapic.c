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

#include <dev/core/ic/i8253reg.h>

#include <arch/i386/isa/isa_machdep.h> 			/* XXX intrhand */

#include <arch/i386/include/cpu.h>
#include <arch/i386/include/pic.h>
#include <arch/i386/include/intr.h>
#include <arch/i386/include/pio.h>
#include <arch/i386/include/pmap.h>
#include <arch/i386/include/pte.h>
#include <arch/i386/include/specialreg.h>
#include <arch/i386/apic/apic.h>
#include <arch/i386/apic/lapicreg.h>
#include <arch/i386/apic/lapicvar.h>
#include <arch/i386/include/mpconfig.h>

#define lapic_lock_init(lock) 	simple_lock_init(lock, "lapic_lock")
#define lapic_lock(lock) 		simple_lock(lock)
#define lapic_unlock(lock) 		simple_unlock(lock)

extern volatile vaddr_t local_apic_va;

void			lapic_map(caddr_t);
static void 	lapic_hwmask(struct softpic *, int);
static void 	lapic_hwunmask(struct softpic *, int);
static void 	lapic_setup(struct softpic *, struct cpu_info *, int, int, int);

static void		x2apic_write_icr(uint32_t hi, uint32_t lo);

struct pic lapic_template = {
		.pic_type = PIC_LAPIC,
		.pic_hwmask = lapic_hwmask,
		.pic_hwunmask = lapic_hwunmask,
		.pic_addroute = lapic_setup,
		.pic_delroute = lapic_setup,
		.pic_register = lapic_register_pic
};

static int i82489_ipi(int vec, int target, int dl);
static int x2apic_ipi(int vec, int target, int dl);
int (*i386_ipi)(int, int, int) = i82489_ipi;

#ifdef LAPIC_ENABLE_X2APIC
bool x2apic_enable = true;
#else
bool x2apic_enable = false;
#endif

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
	*pte = (lapic_base | PG_RW | PG_V | PG_N | PG_G | PG_NX | PG_W | PG_NC_PCD);
	invlpg(va);

	lapic_enable_x2apic();
#ifdef SMP
	cpu_init_first();	/* catch up to changed cpu_number() */
#endif

	lapic_write_tpri(0);
	enable_intr();
}

/*
 * enable local apic
 */
void
lapic_enable(void)
{
	lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | LAPIC_SPURIOUS_VECTOR);
}

void
lapic_set_lvt(void)
{
	struct cpu_info *ci = curcpu();
	int i;
	struct mp_intr_map *mpi;
	uint32_t lint0, lint1;

#ifdef SMP
	if (mp_verbose) {
		apic_format_redir(ci->cpu_dev->dv_xname, "prelint", 0, APIC_VECTYPE_LAPIC_LVT, 0, lapic_read(LAPIC_LVINT0));
		apic_format_redir(ci->cpu_dev->dv_xname, "prelint", 1, APIC_VECTYPE_LAPIC_LVT, 0, lapic_read(LAPIC_LVINT1));
	}
#endif

	/*
	 * If an I/O APIC has been attached, assume that it is used instead of
	 * the 8259A for interrupt delivery.  Otherwise request the LAPIC to
	 * get external interrupts via LINT0 for the primary CPU.
	 */
	lint0 = LAPIC_DLMODE_EXTINT;
	if (nioapics > 0 || !cpu_is_primary(curcpu())) {
		lint0 |= LAPIC_LVT_MASKED;
	}
	lapic_write(LAPIC_LVINT0, lint0);

	/*
	 * Non Maskable Interrupts are to be delivered to the primary CPU.
	 */
	lint1 = LAPIC_DLMODE_NMI;
	if (!cpu_is_primary(curcpu())) {
		lint1 |= LAPIC_LVT_MASKED;
	}
	lapic_write(LAPIC_LVINT1, lint1);

	for (i = 0; i < mp_nintr; i++) {
		mpi = &mp_intrs[i];
		if (mpi->ioapic == NULL && (mpi->cpu_id == MPS_ALL_APICS || mpi->cpu_id == ci->cpu_cpuid)) {
			if (mpi->ioapic_pin > 1)
				panic("lapic_set_lvt: bad pin value %d", mpi->ioapic_pin);
			if (mpi->ioapic_pin == 0) {
				lapic_write(LAPIC_LVINT0, mpi->redir);
			} else {
				lapic_write(LAPIC_LVINT1, mpi->redir);
			}
		}
	}

#ifdef SMP
	if (mp_verbose) {
			apic_format_redir (ci->cpu_dev->dv_xname, "timer", 0, 0, lapic_read(LAPIC_LVTT));
			apic_format_redir (ci->cpu_dev->dv_xname, "pcint", 0, 0, lapic_read(LAPIC_PCINT));
			apic_format_redir (ci->cpu_dev->dv_xname, "lint", 0, 0, lapic_read(LAPIC_LVINT0));
			apic_format_redir (ci->cpu_dev->dv_xname, "lint", 1, 0, lapic_read(LAPIC_LVINT1));
			apic_format_redir (ci->cpu_dev->dv_xname, "err", 0, 0, lapic_read(LAPIC_LVERR));
		}
#endif
}

/*
 * Initialize fixed idt vectors for use by local apic.
 */
void
lapic_boot_init(caddr_t lapic_base)
{
	lapic_map(lapic_base);

	if(x2apic_mode) {
#ifdef SMP
		setidt(LAPIC_IPI_VECTOR, &IDTVEC(x2apic_intr_ipi), 0, SDT_SYS386IGT, SEL_KPL);
		setidt(LAPIC_TLB_VECTOR, &IDTVEC(x2apic_intr_tlb), 0, SDT_SYS386IGT, SEL_KPL);
#endif
		setidt(LAPIC_TIMER_VECTOR, &IDTVEC(x2apic_intr_ltimer), 0, SDT_SYS386IGT, SEL_KPL);
	} else {
#ifdef SMP
		setidt(LAPIC_IPI_VECTOR, &IDTVEC(lapic_intr_ipi), 0, SDT_SYS386IGT, SEL_KPL);
		setidt(LAPIC_TLB_VECTOR, &IDTVEC(lapic_intr_tlb), 0, SDT_SYS386IGT, SEL_KPL);
#endif
		setidt(LAPIC_TIMER_VECTOR, &IDTVEC(lapic_intr_ltimer), 0, SDT_SYS386IGT, SEL_KPL);
	}
	setidt(LAPIC_SPURIOUS_VECTOR, &IDTVEC(spurious), 0, SDT_SYS386IGT, SEL_KPL);
}

static inline u_int32_t
lapic_gettick()
{
	return (lapic_read(LAPIC_CCR_TIMER));
}

int lapic_timer = 0;
u_int32_t lapic_tval;

/*
 * this gets us up to a 4GHz busclock....
 */
u_int32_t lapic_per_second;
u_int32_t lapic_frac_usec_per_cycle;
u_int64_t lapic_frac_cycle_per_usec;
u_int32_t lapic_delaytab[26];

void
lapic_clockintr(void *arg)
{
	struct clockframe *frame = arg;
	hardclock(frame, arg);
}

void
lapic_initclocks()
{
	/*
	 * Start local apic countdown timer running, in repeated mode.
	 *
	 * Mask the clock interrupt and set mode,
	 * then set divisor,
	 * then unmask and set the vector.
	 */
	lapic_write(LAPIC_LVTT, LAPIC_LVTT_TM | LAPIC_LVTT_M);
	lapic_write(LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
	lapic_write(LAPIC_ICR_TIMER, lapic_tval);
	lapic_write(LAPIC_LVTT, LAPIC_LVTT_TM | LAPIC_TIMER_VECTOR);
}

extern int gettick (void);	/* XXX put in header file */

/*
 * Calibrate the local apic count-down timer (which is running at
 * bus-clock speed) vs. the i8254 counter/timer (which is running at
 * a fixed rate).
 *
 * The Intel MP spec says: "An MP operating system may use the IRQ8
 * real-time clock as a reference to determine the actual APIC timer clock
 * speed."
 *
 * We're actually using the IRQ0 timer.  Hmm.
 */
void
lapic_calibrate_timer(ci)
	struct cpu_info *ci;
{
	unsigned int starttick, tick1, tick2, endtick;
	unsigned int startapic, apic1, apic2, endapic;
	u_int64_t dtick, dapic, tmp;
	int i;

	printf("%s: calibrating local timer\n", ci->cpu_dev->dv_xname);

	/*
	 * Configure timer to one-shot, interrupt masked,
	 * large positive number.
	 */
	i82489_write32 (LAPIC_LVTT, LAPIC_LVTT_M);
	i82489_write32 (LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
	i82489_write32(LAPIC_ICR_TIMER, 0x80000000);

	starttick = gettick();
	startapic = lapic_gettick();

	for (i=0; i<hz; i++) {
		delay(2);
		do {
			tick1 = gettick();
			apic1 = lapic_gettick();
		} while (tick1 < starttick);
		delay(2);
		do {
			tick2 = gettick();
			apic2 = lapic_gettick();
		} while (tick2 > starttick);
	}

	endtick = gettick();
	endapic = lapic_gettick();

	dtick = hz * TIMER_DIV(hz);
	dapic = startapic-endapic;

	/*
	 * there are TIMER_FREQ ticks per second.
	 * in dtick ticks, there are dapic bus clocks.
	 */
	tmp = (TIMER_FREQ * dapic) / dtick;

	lapic_per_second = tmp;

	printf("%s: apic clock running at %s\n", ci->cpu_dev->dv_xname, tmp / (1000 * 1000));

	if (lapic_per_second != 0) {
		/*
		 * reprogram the apic timer to run in periodic mode.
		 * XXX need to program timer on other CPUs, too.
		 */
		lapic_tval = (lapic_per_second * 2) / hz;
		lapic_tval = (lapic_tval / 2) + (lapic_tval & 0x1);

		i82489_write32 (LAPIC_LVTT, LAPIC_LVTT_TM | LAPIC_LVTT_M | LAPIC_TIMER_VECTOR);
		i82489_write32 (LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
		i82489_write32 (LAPIC_ICR_TIMER, lapic_tval);

		/*
		 * Compute fixed-point ratios between cycles and
		 * microseconds to avoid having to do any division
		 * in lapic_delay and lapic_microtime.
		 */

		tmp = (1000000 * (u_int64_t)1<<32) / lapic_per_second;
		lapic_frac_usec_per_cycle = tmp;

		tmp = (lapic_per_second * (u_int64_t)1<<32) / 1000000;

		lapic_frac_cycle_per_usec = tmp;

		/*
		 * Compute delay in cycles for likely short delays in usec.
		 */
		for (i=0; i<26; i++) {
			lapic_delaytab[i] = (lapic_frac_cycle_per_usec * i) >> 32;
		}

		/*
		 * Now that the timer's calibrated, use the apic timer routines
		 * for all our timing needs..
		 */
		delay_func = lapic_delay;
		initclock_func = lapic_initclocks;
	}
}

/*
 * delay for N usec.
 */

void lapic_delay(usec)
	int usec;
{
	int32_t tick, otick;
	int64_t deltat;		/* XXX may want to be 64bit */

	otick = lapic_gettick();

	if (usec <= 0)
		return;
	if (usec <= 25) {
		deltat = lapic_delaytab[usec];
	} else {
		deltat = (lapic_frac_cycle_per_usec * usec) >> 32;
	}

	while (deltat > 0) {
		tick = lapic_gettick();
		if (tick > otick) {
			deltat -= lapic_tval - (tick - otick);
		} else {
			deltat -= otick - tick;
		}
		otick = tick;
	}
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
lapic_hwmask(spic, pin)
	struct softpic *spic;
	int pin;
{
	int reg;
	u_int32_t val;
	softpic_pic_hwmask(spic, pin, TRUE, PIC_LAPIC);

	reg = LAPIC_PIN_MASK(LAPIC_LVTT, pin);
	val = lapic_read(reg);
	val |= LAPIC_LVT_MASKED;
	lapic_write(reg, val);
}

static void
lapic_hwunmask(spic, pin)
	struct softpic *spic;
	int pin;
{
	int reg;
	u_int32_t val;
	softpic_pic_hwunmask(spic, pin, TRUE, PIC_LAPIC);

	reg = LAPIC_PIN_MASK(LAPIC_LVTT, pin);
	val = lapic_read(reg);
	val &= ~LAPIC_LVT_MASKED;
	lapic_write(reg, val);
}

static void
lapic_setup(spic, ci, pin, idtvec, type)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type;
{

}

/*
 * Register Local APIC interrupt pins.
 */
static void
lapic_register_pic()
{
	softpic_register_pic(&lapic_template);
}

static void
i82489_icr_wait(void)
{
#ifdef DIAGNOSTIC
	unsigned j = 100000;
#endif /* DIAGNOSTIC */

	while ((i82489_read32(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) != 0) {
		cpu_pause();
#ifdef DIAGNOSTIC
		j--;
		if (j == 0)
			panic("i82489_icr_wait: busy");
#endif /* DIAGNOSTIC */
	}
}

static int
i82489_ipi_init(int target)
{
	uint32_t esr;

	i82489_write32(LAPIC_ESR, 0);
	(void)i82489_read32(LAPIC_ESR);

	i82489_write32(LAPIC_ICRHI, target << LAPIC_ID_SHIFT);

	i82489_write32(LAPIC_ICRLO, LAPIC_DLMODE_INIT | LAPIC_LVL_ASSERT);
	i82489_icr_wait();
	delay_func(10000);
	i82489_write32(LAPIC_ICRLO, LAPIC_DLMODE_INIT | LAPIC_LVL_TRIG | LAPIC_LVL_DEASSERT);
	i82489_icr_wait();

	if ((i82489_read32(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) != 0) {
		return (EBUSY);
	}

	esr = i82489_read32(LAPIC_ESR);
	if (esr != 0) {
		print("%s: ESR %08x\n", __func__, esr);
	}

	return (0);
}

static int
i82489_ipi_startup(int target, int vec)
{
	uint32_t esr;

	i82489_write32(LAPIC_ESR, 0);
	(void)i82489_readreg(LAPIC_ESR);

	i82489_icr_wait();
	i82489_write32(LAPIC_ICRHI, target << LAPIC_ID_SHIFT);
	i82489_write32(LAPIC_ICRLO, vec | LAPIC_DLMODE_STARTUP | LAPIC_LVL_ASSERT);
	i82489_icr_wait();

	if ((i82489_read32(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) != 0) {
		return (EBUSY);
	}

	esr = i82489_read32(LAPIC_ESR);
	if (esr != 0) {
		print("%s: ESR %08x\n", __func__, esr);
	}

	return (0);
}

static int
i82489_ipi(int vec, int target, int dl)
{
	int result, s;

	s = splhigh();

	i82489_icr_wait();

	if ((target & LAPIC_DEST_MASK) == 0) {
		i82489_write32(LAPIC_ICRHI, target << LAPIC_ID_SHIFT);
	}

	i82489_write32(LAPIC_ICRLO, (target & LAPIC_DEST_MASK) | vec | dl | LAPIC_LVL_ASSERT);

#ifdef DIAGNOSTIC
	i82489_icr_wait();
	result = (i82489_read32(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) ? EBUSY : 0;
#else
	/* Don't wait - if it doesn't go, we're in big trouble anyway. */
        result = 0;
#endif
	splx(s);

	return (result);
}

static int
x2apic_ipi_init(int target)
{

	x2apic_write_icr(target, LAPIC_DLMODE_INIT | LAPIC_LVL_ASSERT);

	delay_func(10000);

	x2apic_write_icr(0, LAPIC_DLMODE_INIT | LAPIC_LVL_TRIG | LAPIC_LVL_DEASSERT);

	return (0);
}

static int
x2apic_ipi_startup(int target, int vec)
{
	x2apic_write_icr(target, vec | LAPIC_DLMODE_STARTUP | LAPIC_LVL_ASSERT);

	return (0);
}

static int
x2apic_ipi(int vec, int target, int dl)
{
	uint32_t dest_id = 0;

	if ((target & LAPIC_DEST_MASK) == 0) {
		dest_id = target;
	}

	x2apic_write_icr(dest_id, (target & LAPIC_DEST_MASK) | vec | dl | LAPIC_LVL_ASSERT);

	return (0);
}

int
i386_ipi_init(int target)
{
	if (x2apic_mode) {
		return (x2apic_ipi_init(target));
	}
	return (i82489_ipi_init(target));
}

int
i386_ipi_startup(int target, int vec)
{
	if (x2apic_mode) {
		return (x2apic_ipi_startup(target, vec));
	}
	return (i82489_ipi_startup(target, vec));
}
