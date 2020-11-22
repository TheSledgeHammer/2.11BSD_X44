/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1996, by Steve Passe
 * All rights reserved.
 * Copyright (c) 2003 John Baldwin <jhb@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

/*
 * Local APIC support on Pentium and later processors.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD$"); */

#include <sys/queue.h>
#include <sys/user.h>
#include <arch/i386/include/cputypes.h>
#include <arch/i386/include/segments.h>
#include <arch/i386/include/specialreg.h>

#include <arch/i386/isa/isa_machdep.h>

#include <devel/arch/i386/FBSD/apicreg.h>
#include <devel/arch/i386/FBSD/apicvar.h>

/*
 * I/O interrupts use non-negative IRQ values.  These values are used
 * to mark unused IDT entries or IDT entries reserved for a non-I/O
 * interrupt.
 */
#define	IRQ_FREE		-1
#define	IRQ_TIMER		-2
#define	IRQ_SYSCALL		-3
#define	IRQ_DTRACE_RET	-4
#define	IRQ_EVTCHN		-5

enum lat_timer_mode {
	LAT_MODE_UNDEF =	0,
	LAT_MODE_PERIODIC =	1,
	LAT_MODE_ONESHOT =	2,
	LAT_MODE_DEADLINE =	3,
};

/*
 * Support for local APICs.  Local APICs manage interrupts on each
 * individual processor as opposed to I/O APICs which receive interrupts
 * from I/O devices and then forward them on to the local APICs.
 *
 * Local APICs can also send interrupts to each other thus providing the
 * mechanism for IPIs.
 */

struct lvt {
	u_int 				lvt_edgetrigger:1;
	u_int 				lvt_activehi:1;
	u_int 				lvt_masked:1;
	u_int 				lvt_active:1;
	u_int 				lvt_mode:16;
	u_int 				lvt_vector:8;
};

struct lapic {
	struct lvt 			la_lvts[APIC_LVT_MAX + 1];
	struct lvt 			la_elvts[APIC_ELVT_MAX + 1];
	u_int 				la_id:8;
	u_int 				la_cluster:4;
	u_int 				la_cluster_id:2;
	u_int 				la_present:1;
	u_long 				*la_timer_count;
	uint64_t 			la_timer_period;
	enum lat_timer_mode la_timer_mode;
	uint32_t 			lvt_timer_base;
	uint32_t 			lvt_timer_last;
	/* Include IDT_SYSCALL to make indexing easier. */
	int 				la_ioint_irqs[APIC_NUM_IOINTS + 1];
} static *lapics;

/* Global defaults for local APIC LVT entries. */
static struct lvt lvts[APIC_LVT_MAX + 1] = {
	{ 1, 1, 1, 1, APIC_LVT_DM_EXTINT, 0 },					/* LINT0: masked ExtINT */
	{ 1, 1, 0, 1, APIC_LVT_DM_NMI, 0 },						/* LINT1: NMI */
	{ 1, 1, 1, 1, APIC_LVT_DM_FIXED, APIC_TIMER_INT },		/* Timer */
	{ 1, 1, 0, 1, APIC_LVT_DM_FIXED, APIC_ERROR_INT },		/* Error */
	{ 1, 1, 1, 1, APIC_LVT_DM_NMI, 0 },						/* PMC */
	{ 1, 1, 1, 1, APIC_LVT_DM_FIXED, APIC_THERMAL_INT },	/* Thermal */
	{ 1, 1, 1, 1, APIC_LVT_DM_FIXED, APIC_CMC_INT },		/* CMCI */
};

/* Global defaults for AMD local APIC ELVT entries. */
static struct lvt elvts[APIC_ELVT_MAX + 1] = {
	{ 1, 1, 1, 0, APIC_LVT_DM_FIXED, 0 },
	{ 1, 1, 1, 0, APIC_LVT_DM_FIXED, APIC_CMC_INT },
	{ 1, 1, 1, 0, APIC_LVT_DM_FIXED, 0 },
	{ 1, 1, 1, 0, APIC_LVT_DM_FIXED, 0 },
};

static inthand_t *ioint_handlers[] = {
	NULL,				/* 0 - 31 */
	IDTVEC(apic_isr1),	/* 32 - 63 */
	IDTVEC(apic_isr2),	/* 64 - 95 */
	IDTVEC(apic_isr3),	/* 96 - 127 */
	IDTVEC(apic_isr4),	/* 128 - 159 */
	IDTVEC(apic_isr5),	/* 160 - 191 */
	IDTVEC(apic_isr6),	/* 192 - 223 */
	IDTVEC(apic_isr7),	/* 224 - 255 */
};

static inthand_t *ioint_pti_handlers[] = {
	NULL,					/* 0 - 31 */
	IDTVEC(apic_isr1_pti),	/* 32 - 63 */
	IDTVEC(apic_isr2_pti),	/* 64 - 95 */
	IDTVEC(apic_isr3_pti),	/* 96 - 127 */
	IDTVEC(apic_isr4_pti),	/* 128 - 159 */
	IDTVEC(apic_isr5_pti),	/* 160 - 191 */
	IDTVEC(apic_isr6_pti),	/* 192 - 223 */
	IDTVEC(apic_isr7_pti),	/* 224 - 255 */
};

static u_int32_t lapic_timer_divisors[] = {
	APIC_TDCR_1,
	APIC_TDCR_2,
	APIC_TDCR_4,
	APIC_TDCR_8,
	APIC_TDCR_16,
	APIC_TDCR_32,
	APIC_TDCR_64,
	APIC_TDCR_128
};

extern inthand_t IDTVEC(rsvd_pti), IDTVEC(rsvd);

volatile char 				*lapic_map;
caddr_t 					lapic_paddr;
int 						x2apic_mode;
int 						lapic_eoi_suppression;
static int 					lapic_timer_tsc_deadline;
static u_long 				lapic_timer_divisor, count_freq;
#ifdef SMP
static uint64_t 			lapic_ipi_wait_mult;
#endif
unsigned int 				max_apic_id;

/*
 * Use __nosanitizethread to exempt the LAPIC I/O accessors from KCSan
 * instrumentation.  Otherwise, if x2APIC is not available, use of the global
 * lapic_map will generate a KCSan false positive.  While the mapping is
 * shared among all CPUs, the physical access will always take place on the
 * local CPU's APIC, so there isn't in fact a race here.  Furthermore, the
 * KCSan warning printf can cause a panic if issued during LAPIC access,
 * due to attempted recursive use of event timer resources.
 */

static uint32_t __nosanitizethread
lapic_read32(enum LAPIC_REGISTERS reg)
{
	uint32_t res;

	if (x2apic_mode) {
		res = rdmsr32(MSR_APIC_000 + reg);
	} else {
		res = *(volatile uint32_t *)(lapic_map + reg * LAPIC_MEM_MUL);
	}
	return (res);
}

static void __nosanitizethread
lapic_write32(enum LAPIC_REGISTERS reg, uint32_t val)
{
	if (x2apic_mode) {
		mfence();
		lfence();
		wrmsr(MSR_APIC_000 + reg, val);
	} else {
		*(volatile uint32_t *)(lapic_map + reg * LAPIC_MEM_MUL) = val;
	}
}

static void __nosanitizethread
lapic_write32_nofence(enum LAPIC_REGISTERS reg, uint32_t val)
{
	if (x2apic_mode) {
		wrmsr(MSR_APIC_000 + reg, val);
	} else {
		*(volatile uint32_t *)(lapic_map + reg * LAPIC_MEM_MUL) = val;
	}
}

#ifdef SMP
static uint64_t
lapic_read_icr_lo(void)
{
	return (lapic_read32(LAPIC_ICR_LO));
}

static void
lapic_write_icr(uint32_t vhi, uint32_t vlo)
{
	register_t saveintr;
	uint64_t v;

	if (x2apic_mode) {
		v = ((uint64_t)vhi << 32) | vlo;
		mfence();
		wrmsr(MSR_APIC_000 + LAPIC_ICR_LO, v);
	} else {
		saveintr = intr_disable();
		lapic_write32(LAPIC_ICR_HI, vhi);
		lapic_write32(LAPIC_ICR_LO, vlo);
		intr_restore(saveintr);
	}
}

static void
lapic_write_icr_lo(uint32_t vlo)
{
	if (x2apic_mode) {
		mfence();
		wrmsr(MSR_APIC_000 + LAPIC_ICR_LO, vlo);
	} else {
		lapic_write32(LAPIC_ICR_LO, vlo);
	}
}

static void
lapic_write_self_ipi(uint32_t vector)
{
	KASSERT(x2apic_mode, ("SELF IPI write in xAPIC mode"));
	wrmsr(MSR_APIC_000 + LAPIC_SELF_IPI, vector);
}
#endif /* SMP */

static void
native_lapic_enable_x2apic(void)
{
	uint64_t apic_base;

	apic_base = rdmsr(MSR_APICBASE);
	apic_base |= APICBASE_X2APIC | APICBASE_ENABLED;
	wrmsr(MSR_APICBASE, apic_base);
}

static boolean_t
native_lapic_is_x2apic(void)
{
	uint64_t apic_base;

	apic_base = rdmsr(MSR_APICBASE);
	return ((apic_base & (APICBASE_X2APIC | APICBASE_ENABLED)) == (APICBASE_X2APIC | APICBASE_ENABLED));
}

static void		lapic_enable(void);
static void		lapic_resume(struct pic *pic, bool suspend_cancelled);
static void		lapic_timer_oneshot(struct lapic *);
static void		lapic_timer_oneshot_nointr(struct lapic *, uint32_t);
static void		lapic_timer_periodic(struct lapic *);
static void		lapic_timer_deadline(struct lapic *);
static void		lapic_timer_set_divisor(u_int divisor);
static uint32_t	lvt_mode(struct lapic *la, u_int pin, uint32_t value);
static u_int	apic_idt_to_irq(u_int apic_id, u_int vector);
static void		lapic_set_tpr(u_int vector);

struct pic lapic_pic = {
		.pic_resume = lapic_resume
};

/* Forward declarations for apic_ops */
static void		native_lapic_create(u_int apic_id, int boot_cpu);
static void		native_lapic_init(caddr_t addr);
static void		native_lapic_xapic_mode(void);
static void		native_lapic_setup(int boot);
static void		native_lapic_dump(const char *str);
static void		native_lapic_disable(void);
static void		native_lapic_eoi(void);
static int		native_lapic_id(void);
static int		native_lapic_intr_pending(u_int vector);
static u_int	native_apic_cpuid(u_int apic_id);
static u_int	native_apic_alloc_vector(u_int apic_id, u_int irq);
static u_int	native_apic_alloc_vectors(u_int apic_id, u_int *irqs, u_int count, u_int align);
static void 	native_apic_disable_vector(u_int apic_id, u_int vector);
static void 	native_apic_enable_vector(u_int apic_id, u_int vector);
static void 	native_apic_free_vector(u_int apic_id, u_int vector, u_int irq);
static void 	native_lapic_set_logical_id(u_int apic_id, u_int cluster, u_int cluster_id);
static int 		native_lapic_enable_pmc(void);
static void 	native_lapic_disable_pmc(void);
static void 	native_lapic_reenable_pmc(void);
static void 	native_lapic_enable_cmc(void);
static int 		native_lapic_enable_mca_elvt(void);
static int 		native_lapic_set_lvt_mask(u_int apic_id, u_int lvt, u_char masked);
static int 		native_lapic_set_lvt_mode(u_int apic_id, u_int lvt, uint32_t mode);
static int 		native_lapic_set_lvt_polarity(u_int apic_id, u_int lvt, enum intr_polarity pol);
static int 		native_lapic_set_lvt_triggermode(u_int apic_id, u_int lvt, enum intr_trigger trigger);
#ifdef SMP
static void 	native_lapic_ipi_raw(register_t icrlo, u_int dest);
static void 	native_lapic_ipi_vectored(u_int vector, int dest);
static int 		native_lapic_ipi_wait(int delay);
#endif /* SMP */
static int		native_lapic_ipi_alloc(inthand_t *ipifunc);
static void		native_lapic_ipi_free(int vector);

struct apic_ops apic_ops = {
		.create					= native_lapic_create,
		.init					= native_lapic_init,
		.xapic_mode				= native_lapic_xapic_mode,
		.is_x2apic				= native_lapic_is_x2apic,
		.setup					= native_lapic_setup,
		.dump					= native_lapic_dump,
		.disable				= native_lapic_disable,
		.eoi					= native_lapic_eoi,
		.id						= native_lapic_id,
		.intr_pending			= native_lapic_intr_pending,
		.set_logical_id			= native_lapic_set_logical_id,
		.cpuid					= native_apic_cpuid,
		.alloc_vector			= native_apic_alloc_vector,
		.alloc_vectors			= native_apic_alloc_vectors,
		.enable_vector			= native_apic_enable_vector,
		.disable_vector			= native_apic_disable_vector,
		.free_vector			= native_apic_free_vector,
		.enable_pmc				= native_lapic_enable_pmc,
		.disable_pmc			= native_lapic_disable_pmc,
		.reenable_pmc			= native_lapic_reenable_pmc,
		.enable_cmc				= native_lapic_enable_cmc,
		.enable_mca_elvt		= native_lapic_enable_mca_elvt,
#ifdef SMP
		.ipi_raw				= native_lapic_ipi_raw,
		.ipi_vectored			= native_lapic_ipi_vectored,
		.ipi_wait				= native_lapic_ipi_wait,
#endif
		.ipi_alloc				= native_lapic_ipi_alloc,
		.ipi_free				= native_lapic_ipi_free,
		.set_lvt_mask			= native_lapic_set_lvt_mask,
		.set_lvt_mode			= native_lapic_set_lvt_mode,
		.set_lvt_polarity		= native_lapic_set_lvt_polarity,
		.set_lvt_triggermode	= native_lapic_set_lvt_triggermode,
};

static uint32_t
lvt_mode_impl(struct lapic *la, struct lvt *lvt, u_int pin, uint32_t value)
{
	value &= ~(APIC_LVT_M | APIC_LVT_TM | APIC_LVT_IIPP | APIC_LVT_DM
			| APIC_LVT_VECTOR);
	if (lvt->lvt_edgetrigger == 0)
		value |= APIC_LVT_TM;
	if (lvt->lvt_activehi == 0)
		value |= APIC_LVT_IIPP_INTALO;
	if (lvt->lvt_masked)
		value |= APIC_LVT_M;
	value |= lvt->lvt_mode;
	switch (lvt->lvt_mode) {
	case APIC_LVT_DM_NMI:
	case APIC_LVT_DM_SMI:
	case APIC_LVT_DM_INIT:
	case APIC_LVT_DM_EXTINT:
		if (!lvt->lvt_edgetrigger && bootverbose) {
			printf("lapic%u: Forcing LINT%u to edge trigger\n", la->la_id, pin);
			value &= ~APIC_LVT_TM;
		}
		/* Use a vector of 0. */
		break;
	case APIC_LVT_DM_FIXED:
		value |= lvt->lvt_vector;
		break;
	default:
		panic("bad APIC LVT delivery mode: %#x\n", value);
	}
	return (value);
}

static uint32_t
lvt_mode(struct lapic *la, u_int pin, uint32_t value)
{
	struct lvt *lvt;

	KASSERT(pin <= APIC_LVT_MAX,
	    ("%s: pin %u out of range", __func__, pin));
	if (la->la_lvts[pin].lvt_active)
		lvt = &la->la_lvts[pin];
	else
		lvt = &lvts[pin];

	return (lvt_mode_impl(la, lvt, pin, value));
}

static uint32_t
elvt_mode(struct lapic *la, u_int idx, uint32_t value)
{
	struct lvt *elvt;

	KASSERT(idx <= APIC_ELVT_MAX, ("%s: idx %u out of range", __func__, idx));

	elvt = &la->la_elvts[idx];
	KASSERT(elvt->lvt_active, ("%s: ELVT%u is not active", __func__, idx));
	KASSERT(elvt->lvt_edgetrigger, ("%s: ELVT%u is not edge triggered", __func__, idx));
	KASSERT(elvt->lvt_activehi, ("%s: ELVT%u is not active high", __func__, idx));
	return (lvt_mode_impl(la, elvt, idx, value));
}

/*
 * Map the local APIC and setup necessary interrupt vectors.
 */
static void
native_lapic_init(caddr_t addr)
{
#ifdef SMP
	uint64_t r, r1, r2, rx;
#endif
	uint32_t ver;
	int i;
	bool arat;

	/*
	 * Enable x2APIC mode if possible. Map the local APIC
	 * registers page.
	 *
	 * Keep the LAPIC registers page mapped uncached for x2APIC
	 * mode too, to have direct map page attribute set to
	 * uncached.  This is needed to work around CPU errata present
	 * on all Intel processors.
	 */
	KASSERT(trunc_page(addr) == addr, ("local APIC not aligned on a page boundary"));
	lapic_paddr = addr;
	lapic_map = pmap_mapdev(addr, PAGE_SIZE);  /* TODO: Fix pmap_mapdev */
	if (x2apic_mode) {
		native_lapic_enable_x2apic();
		lapic_map = NULL;
	}

	/* Setup the spurious interrupt handler. */
	setidt(APIC_SPURIOUS_INT, IDTVEC(spuriousint), SDT_APIC, SEL_KPL, GSEL_APIC);

	/* Perform basic initialization of the BSP's local APIC. */
	lapic_enable();

	/* Set BSP's per-CPU local APIC ID. */
	//PCPU_SET(apic_id, lapic_id());

	/* Local APIC timer interrupt. */
	setidt(APIC_TIMER_INT, pti ? IDTVEC(timerint_pti) : IDTVEC(timerint), SDT_APIC, SEL_KPL, GSEL_APIC);

	/* Local APIC error interrupt. */
	setidt(APIC_ERROR_INT, pti ? IDTVEC(errorint_pti) : IDTVEC(errorint), SDT_APIC, SEL_KPL, GSEL_APIC);

	/* XXX: Thermal interrupt */

	/* Local APIC CMCI. */
	setidt(APIC_CMC_INT, pti ? IDTVEC(cmcint_pti) : IDTVEC(cmcint), SDT_APIC, SEL_KPL, GSEL_APIC);

	/*
	 * Set lapic_eoi_suppression after lapic_enable(), to not
	 * enable suppression in the hardware prematurely.  Note that
	 * we by default enable suppression even when system only has
	 * one IO-APIC, since EOI is broadcasted to all APIC agents,
	 * including CPUs, otherwise.
	 *
	 * It seems that at least some KVM versions report
	 * EOI_SUPPRESSION bit, but auto-EOI does not work.
	 */
	ver = lapic_read32(LAPIC_VERSION);

#ifdef SMP
#define	LOOPS	100000
	/*
	 * Calibrate the busy loop waiting for IPI ack in xAPIC mode.
	 * lapic_ipi_wait_mult contains the number of iterations which
	 * approximately delay execution for 1 microsecond (the
	 * argument to native_lapic_ipi_wait() is in microseconds).
	 *
	 * We assume that TSC is present and already measured.
	 * Possible TSC frequency jumps are irrelevant to the
	 * calibration loop below, the CPU clock management code is
	 * not yet started, and we do not enter sleep states.
	 */
	KASSERT((cpu_feature & CPUID_TSC) != 0 && tsc_freq != 0, ("TSC not initialized"));
	if (!x2apic_mode) {
		r = rdtsc();
		for (rx = 0; rx < LOOPS; rx++) {
			(void)lapic_read_icr_lo();
			ia32_pause();
		}
		r = rdtsc() - r;
		r1 = tsc_freq * LOOPS;
		r2 = r * 1000000;
		lapic_ipi_wait_mult = r1 >= r2 ? r1 / r2 : 1;
		if (bootverbose) {
			printf("LAPIC: ipi_wait() us multiplier %ju (r %ju "
			    "tsc %ju)\n", (uintmax_t)lapic_ipi_wait_mult,
			    (uintmax_t)r, (uintmax_t)tsc_freq);
		}
	}
#undef LOOPS
#endif /* SMP */
}

/*
 * Create a local APIC instance.
 */
static void
native_lapic_create(u_int apic_id, int boot_cpu)
{
	int i;

	if (apic_id > max_apic_id) {
		printf("APIC: Ignoring local APIC with ID %d\n", apic_id);
		if (boot_cpu)
			panic("Can't ignore BSP");
		return;
	}
	KASSERT(!lapics[apic_id].la_present, ("duplicate local APIC %u", apic_id));

	/*
	 * Assume no local LVT overrides and a cluster of 0 and
	 * intra-cluster ID of 0.
	 */
	lapics[apic_id].la_present = 1;
	lapics[apic_id].la_id = apic_id;
	for (i = 0; i <= APIC_LVT_MAX; i++) {
		lapics[apic_id].la_lvts[i] = lvts[i];
		lapics[apic_id].la_lvts[i].lvt_active = 0;
	}
	for (i = 0; i <= APIC_ELVT_MAX; i++) {
		lapics[apic_id].la_elvts[i] = elvts[i];
		lapics[apic_id].la_elvts[i].lvt_active = 0;
	}
	for (i = 0; i <= APIC_NUM_IOINTS; i++)
		lapics[apic_id].la_ioint_irqs[i] = IRQ_FREE;
	lapics[apic_id].la_ioint_irqs[IDT_SYSCALL - APIC_IO_INTS] = IRQ_SYSCALL;
	lapics[apic_id].la_ioint_irqs[APIC_TIMER_INT - APIC_IO_INTS] = IRQ_TIMER;
}

static inline uint32_t
amd_read_ext_features(void)
{
	uint32_t version;

	if (cpu_vendor_id != CPUVENDOR_AMD && cpu_vendor_id != CPUVENDOR_HYGON)
		return (0);
	version = lapic_read32(LAPIC_VERSION);
	if ((version & APIC_VER_AMD_EXT_SPACE) != 0)
		return (lapic_read32(LAPIC_EXT_FEATURES));
	else
		return (0);
}

static inline uint32_t
amd_read_elvt_count(void)
{
	uint32_t extf;
	uint32_t count;

	extf = amd_read_ext_features();
	count = (extf & APIC_EXTF_ELVT_MASK) >> APIC_EXTF_ELVT_SHIFT;
	count = min(count, APIC_ELVT_MAX + 1);
	return (count);
}

/*
 * Dump contents of local APIC registers
 */
static void
native_lapic_dump(const char* str)
{
	uint32_t version;
	uint32_t maxlvt;
	uint32_t extf;
	int elvt_count;
	int i;

	version = lapic_read32(LAPIC_VERSION);
	maxlvt = (version & APIC_VER_MAXLVT) >> MAXLVTSHIFT;
	//printf("cpu%d %s:\n", PCPU_GET(cpuid), str);
	printf("     ID: 0x%08x   VER: 0x%08x LDR: 0x%08x DFR: 0x%08x", lapic_read32(LAPIC_ID), version, lapic_read32(LAPIC_LDR), x2apic_mode ? 0 : lapic_read32(LAPIC_DFR));
	if ((cpu_feature2 & CPUID2_X2APIC) != 0)
		printf(" x2APIC: %d", x2apic_mode);
	printf("\n  lint0: 0x%08x lint1: 0x%08x TPR: 0x%08x SVR: 0x%08x\n", lapic_read32(LAPIC_LVT_LINT0), lapic_read32(LAPIC_LVT_LINT1), lapic_read32(LAPIC_TPR), lapic_read32(LAPIC_SVR));
	printf("  timer: 0x%08x therm: 0x%08x err: 0x%08x", lapic_read32(LAPIC_LVT_TIMER), lapic_read32(LAPIC_LVT_THERMAL), lapic_read32(LAPIC_LVT_ERROR));
	if (maxlvt >= APIC_LVT_PMC)
		printf(" pmc: 0x%08x", lapic_read32(LAPIC_LVT_PCINT));
	printf("\n");
	if (maxlvt >= APIC_LVT_CMCI)
		printf("   cmci: 0x%08x\n", lapic_read32(LAPIC_LVT_CMCI));
	extf = amd_read_ext_features();
	if (extf != 0) {
		printf("   AMD ext features: 0x%08x\n", extf);
		elvt_count = amd_read_elvt_count();
		for (i = 0; i < elvt_count; i++)
			printf("   AMD elvt%d: 0x%08x\n", i, lapic_read32(LAPIC_EXT_LVT0 + i));
	}
}

static void
native_lapic_xapic_mode(void)
{
	register_t saveintr;

	saveintr = intr_disable();
	if (x2apic_mode)
		native_lapic_enable_x2apic();
	intr_restore(saveintr);
}

static void
native_lapic_setup(int boot)
{
	struct lapic 	*la;
	uint32_t 		version;
	uint32_t 		maxlvt;
	register_t 		saveintr;
	int 			elvt_count;
	int 			i;

	saveintr = intr_disable();

	la = &lapics[lapic_id()];
	KASSERT(la->la_present, ("missing APIC structure"));
	version = lapic_read32(LAPIC_VERSION);
	maxlvt = (version & APIC_VER_MAXLVT) >> MAXLVTSHIFT;

	/* Initialize the TPR to allow all interrupts. */
	lapic_set_tpr(0);

	/* Setup spurious vector and enable the local APIC. */
	lapic_enable();

	/* Program LINT[01] LVT entries. */
	lapic_write32(LAPIC_LVT_LINT0,
			lvt_mode(la, APIC_LVT_LINT0, lapic_read32(LAPIC_LVT_LINT0)));
	lapic_write32(LAPIC_LVT_LINT1,
			lvt_mode(la, APIC_LVT_LINT1, lapic_read32(LAPIC_LVT_LINT1)));

	/* Program the PMC LVT entry if present. */
	if (maxlvt >= APIC_LVT_PMC) {
		lapic_write32(LAPIC_LVT_PCINT, lvt_mode(la, APIC_LVT_PMC, LAPIC_LVT_PCINT));
	}

	/* Program timer LVT. */
	la->lvt_timer_base = lvt_mode(la, APIC_LVT_TIMER, lapic_read32(LAPIC_LVT_TIMER));
	la->lvt_timer_last = la->lvt_timer_base;
	lapic_write32(LAPIC_LVT_TIMER, la->lvt_timer_base);

	/* Calibrate the timer parameters using BSP. */
	if (boot && IS_BSP()) {
		lapic_calibrate_initcount(la);
		if (lapic_timer_tsc_deadline)
			lapic_calibrate_deadline(la);
	}

	/* Setup the timer if configured. */
	if (la->la_timer_mode != LAT_MODE_UNDEF) {
		KASSERT(la->la_timer_period != 0, ("lapic%u: zero divisor", lapic_id()));
		switch (la->la_timer_mode) {
		case LAT_MODE_PERIODIC:
			lapic_timer_set_divisor(lapic_timer_divisor);
			lapic_timer_periodic(la);
			break;
		case LAT_MODE_ONESHOT:
			lapic_timer_set_divisor(lapic_timer_divisor);
			lapic_timer_oneshot(la);
			break;
		case LAT_MODE_DEADLINE:
			lapic_timer_deadline(la);
			break;
		default:
			panic("corrupted la_timer_mode %p %d", la, la->la_timer_mode);
		}
	}

	/* Program error LVT and clear any existing errors. */
	lapic_write32(LAPIC_LVT_ERROR, lvt_mode(la, APIC_LVT_ERROR, lapic_read32(LAPIC_LVT_ERROR)));
	lapic_write32(LAPIC_ESR, 0);

	/* XXX: Thermal LVT */

	/* Program the CMCI LVT entry if present. */
	if (maxlvt >= APIC_LVT_CMCI) {
		lapic_write32(LAPIC_LVT_CMCI, lvt_mode(la, APIC_LVT_CMCI, lapic_read32(LAPIC_LVT_CMCI)));
	}

	elvt_count = amd_read_elvt_count();
	for (i = 0; i < elvt_count; i++) {
		if (la->la_elvts[i].lvt_active)
			lapic_write32(LAPIC_EXT_LVT0 + i, elvt_mode(la, i, lapic_read32(LAPIC_EXT_LVT0 + i)));
	}

	intr_restore(saveintr);
}

static void
lapic_calibrate_initcount(struct lapic *la)
{
	u_long value;

	/* Start off with a divisor of 2 (power on reset default). */
	lapic_timer_divisor = 2;
	/* Try to calibrate the local APIC timer. */
	do {
		lapic_timer_set_divisor(lapic_timer_divisor);
		lapic_timer_oneshot_nointr(la, APIC_TIMER_MAX_COUNT);
		DELAY(1000000);
		value = APIC_TIMER_MAX_COUNT - lapic_read32(LAPIC_CCR_TIMER);
		if (value != APIC_TIMER_MAX_COUNT)
			break;
		lapic_timer_divisor <<= 1;
	} while (lapic_timer_divisor <= 128);
	if (lapic_timer_divisor > 128)
		panic("lapic: Divisor too big");
	if (bootverbose) {
		printf("lapic: Divisor %lu, Frequency %lu Hz\n", lapic_timer_divisor, value);
	}
	count_freq = value;
}

static void
lapic_calibrate_deadline(struct lapic *la /*__unused*/)
{
	if (bootverbose) {
		printf("lapic: deadline tsc mode, Frequency %ju Hz\n", (uintmax_t)tsc_freq);
	}
}

static void
native_lapic_disable(void)
{
	uint32_t value;

	/* Software disable the local APIC. */
	value = lapic_read32(LAPIC_SVR);
	value &= ~APIC_SVR_SWEN;
	lapic_write32(LAPIC_SVR, value);
}

static void
lapic_enable(void)
{
	uint32_t value;

	/* Program the spurious vector to enable the local APIC. */
	value = lapic_read32(LAPIC_SVR);
	value &= ~(APIC_SVR_VECTOR | APIC_SVR_FOCUS);
	value |= APIC_SVR_FEN | APIC_SVR_SWEN | APIC_SPURIOUS_INT;
	if (lapic_eoi_suppression)
		value |= APIC_SVR_EOI_SUPPRESSION;
	lapic_write32(LAPIC_SVR, value);
}

/* Reset the local APIC on the BSP during resume. */
static void
lapic_resume(struct pic *pic, bool suspend_cancelled)
{
	lapic_setup(0);
}

static int
native_lapic_id(void)
{
	uint32_t v;

	KASSERT(x2apic_mode || lapic_map != NULL, ("local APIC is not mapped"));
	v = lapic_read32(LAPIC_ID);
	if (!x2apic_mode)
		v >>= APIC_ID_SHIFT;
	return (v);
}


static void
lapic_timer_set_divisor(u_int divisor)
{
	KASSERT(powerof2(divisor), ("lapic: invalid divisor %u", divisor));
	KASSERT(ffs(divisor) <= nitems(lapic_timer_divisors), ("lapic: invalid divisor %u", divisor));
	lapic_write32(LAPIC_DCR_TIMER, lapic_timer_divisors[ffs(divisor) - 1]);
}

static void
lapic_timer_oneshot(struct lapic *la)
{
	uint32_t value;

	value = la->lvt_timer_base;
	value &= ~(APIC_LVTT_TM | APIC_LVT_M);
	value |= APIC_LVTT_TM_ONE_SHOT;
	la->lvt_timer_last = value;
	lapic_write32(LAPIC_LVT_TIMER, value);
	lapic_write32(LAPIC_ICR_TIMER, la->la_timer_period);
}

static void
lapic_timer_oneshot_nointr(struct lapic *la, uint32_t count)
{
	uint32_t value;

	value = la->lvt_timer_base;
	value &= ~APIC_LVTT_TM;
	value |= APIC_LVTT_TM_ONE_SHOT | APIC_LVT_M;
	la->lvt_timer_last = value;
	lapic_write32(LAPIC_LVT_TIMER, value);
	lapic_write32(LAPIC_ICR_TIMER, count);
}

static void
lapic_timer_periodic(struct lapic *la)
{
	uint32_t value;

	value = la->lvt_timer_base;
	value &= ~(APIC_LVTT_TM | APIC_LVT_M);
	value |= APIC_LVTT_TM_PERIODIC;
	la->lvt_timer_last = value;
	lapic_write32(LAPIC_LVT_TIMER, value);
	lapic_write32(LAPIC_ICR_TIMER, la->la_timer_period);
}

static void
lapic_timer_deadline(struct lapic *la)
{
	uint32_t value;

	value = la->lvt_timer_base;
	value &= ~(APIC_LVTT_TM | APIC_LVT_M);
	value |= APIC_LVTT_TM_TSCDLT;
	if (value != la->lvt_timer_last) {
		la->lvt_timer_last = value;
		lapic_write32_nofence(LAPIC_LVT_TIMER, value);
		if (!x2apic_mode)
			mfence();
	}
	wrmsr(MSR_TSC_DEADLINE, la->la_timer_period + rdtsc());
}

