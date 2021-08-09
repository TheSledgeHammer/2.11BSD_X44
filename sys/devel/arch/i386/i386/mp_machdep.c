/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
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

#include <devel/sys/percpu.h>

#include <arch/i386/include/cpu.h>
#include <arch/i386/include/cputypes.h>
#include <arch/i386/include/pcb.h>
#include <arch/i386/include/psl.h>
#include <arch/i386/include/pte.h>
#include <arch/i386/include/param.h>
#include <arch/i386/include/specialreg.h>
#include <arch/i386/include/vmparam.h>
#include <arch/i386/include/vm86.h>

#include <devel/arch/i386/include/smp.h>
#include <devel/arch/i386/include/percpu.h>

#define WARMBOOT_TARGET		0
#define WARMBOOT_OFF		(PMAP_MAP_LOW + 0x0467)
#define WARMBOOT_SEG		(PMAP_MAP_LOW + 0x0469)

#define CMOS_REG			(0x70)
#define CMOS_DATA			(0x71)
#define BIOS_RESET			(0x0f)
#define BIOS_WARM			(0x0a)

/*
 * this code MUST be enabled here and in mpboot.s.
 * it follows the very early stages of AP boot by placing values in CMOS ram.
 * it NORMALLY will never be needed and thus the primitive method for enabling.
 *
#define CHECK_POINTS
 */

#if defined(CHECK_POINTS)
#define CHECK_READ(A)		(outb(CMOS_REG, (A)), inb(CMOS_DATA))
#define CHECK_WRITE(A,D) 	(outb(CMOS_REG, (A)), outb(CMOS_DATA, (D)))

#define CHECK_INIT(D);				\
	CHECK_WRITE(0x34, (D));			\
	CHECK_WRITE(0x35, (D));			\
	CHECK_WRITE(0x36, (D));			\
	CHECK_WRITE(0x37, (D));			\
	CHECK_WRITE(0x38, (D));			\
	CHECK_WRITE(0x39, (D));

#define CHECK_PRINT(S);				\
	printf("%s: %d, %d, %d, %d, %d, %d\n",	\
	(S),							\
	CHECK_READ(0x34),				\
	CHECK_READ(0x35),				\
	CHECK_READ(0x36),				\
	CHECK_READ(0x37),				\
	CHECK_READ(0x38),				\
	CHECK_READ(0x39));

#else				/* CHECK_POINTS */

#define CHECK_INIT(D)
#define CHECK_PRINT(S)
#define CHECK_WRITE(A, D)

#endif				/* CHECK_POINTS */

/*
 * Local data and functions.
 */

static void	install_ap_tramp(void);
static int	start_all_aps(void);
static int	start_ap(int apic_id);

/* Set to 1 once we're ready to let the APs out of the pen. */
static volatile int aps_ready = 0;

/*
 * Initialize the IPI handlers and start up the AP's.
 */
void
cpu_mp_start(pc)
	struct percpu *pc;
{
	int i;
	for (i = 0; i < NCPUS; i++) {
		cpu_apic_ids[i] = -1;
	}

	/* Install an inter-CPU IPI for TLB invalidation */
	setidt(IPI_INVLTLB, &IDTVEC(invltlb), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(IPI_INVLPG, &IDTVEC(invlpg), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(IPI_INVLRNG, &IDTVEC(invlrng), 0, SDT_SYS386IGT, SEL_KPL);

	/* Set boot_cpu_id if needed. */
	if (boot_cpu_id == -1) {
		boot_cpu_id = PERCPU_GET(pc, apic_id);
		cpu_info[boot_cpu_id].cpu_bsp = 1;
	} else {
		KASSERT(boot_cpu_id == PERCPU_GET(pc, apic_id)("BSP's APIC ID doesn't match boot_cpu_id"));
	}

	/* Probe logical/physical core configuration. */
	topo_probe();

	assign_cpu_ids();

	/* Start each Application Processor */
	start_all_aps();

	set_interrupt_apic_ids();
}

void
init_secondary(ci)
	struct cpu_info *ci;
{
	struct percpu *pc;
	struct i386tss *common_tssp;
	struct region_descriptor r_gdt, r_idt;
	int gsel_tss, myid, x;
	u_int cr0;

	/* Get per-cpu data */
	pc = ci->cpu_percpu = &__percpu[myid];

	/* prime data page for it to use */
	percpu_init(pc, ci, sizeof(struct percpu));
	pc->pc_apic_id = cpu_apic_ids[myid];
	pc->pc_curkthread = 0;
	pc->pc_cpuinfo = ci;
	pc->pc_cpuinfo->cpu_percpu = pc = ci->cpu_percpu;
	pc->pc_common_tssp = common_tssp = &(__percpu[0].pc_common_tssp)[myid];

	fix_cpuid();

	gdt_segs[GPRIV_SEL].ssd_base = (int)pc;
	gdt_segs[GPROC0_SEL].ssd_base = (int)common_tssp;
	gdt_segs[GLDT_SEL].ssd_base = (int)ldt;

	for (x = 0; x < NGDT; x++) {
		ssdtosd(&gdt_segs[x], &gdt[myid * NGDT + x].sd);
	}

	r_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
	r_gdt.rd_base = (int) &gdt[myid * NGDT];
	lgdt(&r_gdt);									/* does magic intra-segment return */

	r_idt.rd_limit = sizeof(struct gate_descriptor) * NIDT - 1;
	r_idt.rd_base = (int)idt;
	lidt(&r_idt);

	lldt(_default_ldt);
	PERCPU_SET(pc, currentldt, _default_ldt);

	gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	gdt[myid * NGDT + GPROC0_SEL].sd.sd_type = SDT_SYS386TSS;
	common_tssp->tss_esp0 = PERCPU_GET(pc, trampstk);
	common_tssp->tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	common_tssp->tss_ioopt = sizeof(struct i386tss) << 16;
	PERCPU_SET(pc, tss_gdt, &gdt[myid * NGDT + GPROC0_SEL].sd);
	PERCPU_SET(pc, common_tssd, PERCPU_GET(pc, tss_gdt));
	ltr(gsel_tss);

	PERCPU_SET(pc, fsgs_gdt, &gdt[myid * NGDT + GUFS_SEL].sd);

	/*
	 * Set to a known state:
	 * Set by mpboot.s: CR0_PG, CR0_PE
	 * Set by cpu_setregs: CR0_NE, CR0_MP, CR0_TS, CR0_WP, CR0_AM
	 */
	cr0 = rcr0();
	cr0 &= ~(CR0_CD | CR0_NW | CR0_EM);
	lcr0(cr0);
	CHECK_WRITE(0x38, 5);

	/* signal our startup to the BSP. */
	mp_naps++;
	CHECK_WRITE(0x39, 6);

	/* Spin until the BSP releases the AP's. */
	while (!aps_ready) {
		ia32_pause();
	}

	/* BSP may have changed PTD while we were waiting */
	invltlb();
	pmap_invalidate_range(kernel_pmap, 0, NKPT * NBPDR - 1);

#if defined(I586_CPU) && !defined(NO_F00F_HACK)
	lidt(&r_idt);
#endif

	init_secondary_tail(pc);
}

/*
 * start each AP in our list
 */
#define TMPMAP_START 1
static int
start_all_aps(void)
{
	uintptr_t kptbase;
	u_char mpbiosreason;
	u_int32_t mpbioswarmvec;
	int apic_id, cpu, i;

	pmap_remap_lower(TRUE);

	/* install the AP 1st level boot code */
	install_ap_tramp();

	/* save the current value of the warm-start vector */
	mpbioswarmvec = *((u_int32_t *) WARMBOOT_OFF);
	outb(CMOS_REG, BIOS_RESET);
	mpbiosreason = inb(CMOS_DATA);

	/* take advantage of the P==V mapping for PTD[0] for AP boot */

	/* start each AP */
	for (cpu = 1; cpu < mp_ncpus; cpu++) {
		apic_id = cpu_apic_ids[cpu];

		/* allocate and set up a boot stack data page */
		bootstacks[cpu] = (char*) kmem_alloc(kernel_map, KSTACK_PAGES * PAGE_SIZE);
		/* setup a vector to our boot code */
		*((volatile u_short*) WARMBOOT_OFF) = WARMBOOT_TARGET;
		*((volatile u_short*) WARMBOOT_SEG) = (boot_address >> 4);
		outb(CMOS_REG, BIOS_RESET);
		outb(CMOS_DATA, BIOS_WARM); /* 'warm-start' */

		bootSTK = (char*) bootstacks[cpu] + KSTACK_PAGES * PAGE_SIZE - 4;
		bootAP = cpu;

		/* attempt to start the Application Processor */
		CHECK_INIT(99); /* setup checkpoints */
		if (!start_ap(apic_id)) {
			printf("AP #%d (PHY# %d) failed!\n", cpu, apic_id);
			CHECK_PRINT("trace"); /* show checkpoints */
			/* better panic as the AP may be running loose */
			printf("panic y/n? [y] ");
			if (cngetc() != 'n') {
				panic("bye-bye");
			}
		}
		CHECK_PRINT("trace"); /* show checkpoints */
	}

	pmap_remap_lower(FALSE);

	/* restore the warmstart vector */
	*(u_int32_t *) WARMBOOT_OFF = mpbioswarmvec;

	outb(CMOS_REG, BIOS_RESET);
	outb(CMOS_DATA, mpbiosreason);

	/* number of APs actually started */
	return (mp_naps);
}

/*
 * load the 1st level AP boot code into base memory.
 */

/* targets for relocation */
extern void bigJump(void);
extern void bootCodeSeg(void);
extern void bootDataSeg(void);
extern void MPentry(void);
extern u_int MP_GDT;
extern u_int mp_gdtbase;

static void
install_ap_tramp(void)
{
	int x;
	int size = *(int*) ((u_long) &bootMP_size);
	vm_offset_t va = boot_address;
	u_char *src = (u_char*) ((u_long) bootMP);
	u_char *dst = (u_char*) va;
	u_int boot_base = (u_int) bootMP;
	u_int8_t *dst8;
	u_int16_t *dst16;
	u_int32_t *dst32;

	KASSERT (size <= PAGE_SIZE ("'size' do not fit into PAGE_SIZE, as expected."));
	pmap_kenter(va, boot_address);
	pmap_invalidate_page(kernel_pmap, va);
	for (x = 0; x < size; ++x) {
		*dst++ = *src++;
	}

	/*
	 * modify addresses in code we just moved to basemem. unfortunately we
	 * need fairly detailed info about mpboot.s for this to work.  changes
	 * to mpboot.s might require changes here.
	 */

	/* boot code is located in KERNEL space */
	dst = (u_char*) va;

	/* modify the lgdt arg */
	dst32 = (u_int32_t*) (dst + ((u_int) &mp_gdtbase - boot_base));
	*dst32 = boot_address + ((u_int) &MP_GDT - boot_base);

	/* modify the ljmp target for MPentry() */
	dst32 = (u_int32_t*) (dst + ((u_int) bigJump - boot_base) + 1);
	*dst32 = (u_int) MPentry;

	/* modify the target for boot code segment */
	dst16 = (u_int16_t*) (dst + ((u_int) bootCodeSeg - boot_base));
	dst8 = (u_int8_t*) (dst16 + 1);
	*dst16 = (u_int) boot_address & 0xffff;
	*dst8 = ((u_int) boot_address >> 16) & 0xff;

	/* modify the target for boot data segment */
	dst16 = (u_int16_t*) (dst + ((u_int) bootDataSeg - boot_base));
	dst8 = (u_int8_t*) (dst16 + 1);
	*dst16 = (u_int) boot_address & 0xffff;
	*dst8 = ((u_int) boot_address >> 16) & 0xff;
}

/*
 * This function starts the AP (application processor) identified
 * by the APIC ID 'physicalCpu'.  It does quite a "song and dance"
 * to accomplish this.  This is necessary because of the nuances
 * of the different hardware we might encounter.  It isn't pretty,
 * but it seems to work.
 */
static int
start_ap(int apic_id)
{
	int vector, ms, cpus, error;

	/* calculate the vector */
	vector = (boot_address >> 12) & 0xff;

	/* used as a watchpoint to signal AP startup */
	cpus = mp_naps;

	ipi_startup(apic_id, vector);

	/* Wait up to 5 seconds for it to start. */
	for (ms = 0; ms < 5000; ms++) {
		if (mp_naps > cpus) {
			return (1); /* return SUCCESS */
		}
		delay_func(1000);
	}
	return (0); /* return FAILURE */
}

static void
ipi_startup(apic_id, vector)
	int apic_id, vector;
{
	int error;
	error = i386_ipi_init(apic_id);
	if (error != 0) {
		printf("i386_ipi_init was unsuccessful: %d apic_id: \n", apic_id);
	}
	delay_func(10000);

	error = i386_ipi_startup(apic_id, vector);
	if (error != 0) {
		printf("i386_ipi_startup was unsuccessful: %d apic_id: %d vector: \n", apic_id, vector);
	}
	delay_func(200);
}

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

	simple_lock(&smp_tlb_lock);
	smp_tlb_addr1 = addr1;
	smp_tlb_addr2 = addr2;
	atomic_store_rel_int(&smp_tlb_wait, 0);

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
	simple_unlock(&smp_tlb_lock);
	return;
}

void
smp_masked_invltlb(u_int mask, pmap_t pmap)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLTLB, pmap, 0, 0);
	}
}

void
smp_masked_invlpg(u_int mask, vm_offset_t addr, pmap_t pmap)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLPG, pmap, addr, 0);
	}
}

void
smp_masked_invlpg_range(u_int mask, vm_offset_t addr1, vm_offset_t addr2, pmap_t pmap)
{
	if (smp_started) {
		smp_targeted_tlb_shootdown(mask, IPI_INVLRNG, pmap, addr1, addr2);
	}
}
