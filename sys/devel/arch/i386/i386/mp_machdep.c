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
#include <sys/percpu.h>
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
#include <arch/i386/include/param.h>
#include <arch/i386/include/specialreg.h>
#include <arch/i386/include/vm86.h>

#include <devel/arch/i386/include/i386_smp.h>
#include <devel/arch/i386/include/cpu.h>
//#include <devel/arch/i386/include/percpu.h>

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
	   CHECK_READ(0x34),			\
	   CHECK_READ(0x35),			\
	   CHECK_READ(0x36),			\
	   CHECK_READ(0x37),			\
	   CHECK_READ(0x38),			\
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

static char *ap_copyout_buf;
static char *ap_tramp_stack_base;

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
	setidt(IPI_INVLTLB, &IDTVEC(invltlb), SDT_SYS386IGT, SEL_KPL);
	setidt(IPI_INVLPG, &IDTVEC(invlpg), SDT_SYS386IGT, SEL_KPL);
	setidt(IPI_INVLRNG, IDTVEC(invlrng), SDT_SYS386IGT, SEL_KPL);

	/* Install an inter-CPU IPI for cache invalidation. */
	setidt(IPI_INVLCACHE, IDTVEC(invlcache), SDT_SYS386IGT, SEL_KPL);

	/* Install an inter-CPU IPI for all-CPU rendezvous */
	setidt(IPI_RENDEZVOUS, IDTVEC(rendezvous), SDT_SYS386IGT, SEL_KPL);

	/* Install generic inter-CPU IPI handler */
	setidt(IPI_BITMAP_VECTOR, IDTVEC(ipi_intr_bitmap_handler), SDT_SYS386IGT, SEL_KPL);

	/* Install an inter-CPU IPI for CPU stop/restart */
	setidt(IPI_STOP, IDTVEC(cpustop), SDT_SYS386IGT, SEL_KPL);

	/* Install an inter-CPU IPI for CPU suspend/resume */
	setidt(IPI_SUSPEND, IDTVEC(cpususpend), SDT_SYS386IGT, SEL_KPL);

	/* Install an IPI for calling delayed SWI */
	setidt(IPI_SWI, IDTVEC(ipi_swi), SDT_SYS386IGT, SEL_KPL);

	/* Set boot_cpu_id if needed. */
	if (boot_cpu_id == -1) {
		boot_cpu_id = PERCPU_GET(pc, apic_id);
		cpu_info[boot_cpu_id].cpu_bsp = 1;
	} else {
		KASSERT(boot_cpu_id == PERCPU_GET(pc, apic_id)("BSP's APIC ID doesn't match boot_cpu_id"));
	}
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
	pc->pc_cpuinfo->cpu_percpu = ci->cpu_percpu;
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
	lgdt(&r_gdt);			/* does magic intra-segment return */

	r_idt.rd_limit = sizeof(struct gate_descriptor) * NIDT - 1;
	r_idt.rd_base = (int)idt;
	lidt(&r_idt);

	lldt(_default_ldt);
	PERCPU_SET(pc, currentldt, _default_ldt);

	PERCPU_SET(pc, trampstk, (uintptr_t)ap_tramp_stack_base + TRAMP_STACK_SZ - VM86_STACK_SPACE);

	gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	gdt[myid * NGDT + GPROC0_SEL].sd.sd_type = SDT_SYS386TSS;
	common_tssp->tss_esp0 = PERCPU_GET(pc, trampstk);
	common_tssp->tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	common_tssp->tss_ioopt = sizeof(struct i386tss) << 16;
	PERCPU_SET(pc, tss_gdt, &gdt[myid * NGDT + GPROC0_SEL].sd);
	PERCPU_SET(pc, common_tssd, PERCPU_GET(pc, tss_gdt));
	ltr(gsel_tss);

	PERCPU_SET(pc, fsgs_gdt, &gdt[myid * NGDT + GUFS_SEL].sd);
	PERCPU_SET(pc, copyout_buf, ap_copyout_buf);

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
	while (atomic_load_acq_int(&aps_ready) == 0) { 			/* XXX: doesn't exist */
		ia32_pause();
	}

	/* BSP may have changed PTD while we were waiting */
	invltlb();

#if defined(I586_CPU) && !defined(NO_F00F_HACK)
	lidt(&r_idt);
#endif

	init_secondary_tail();
}

/*
 * start each AP in our list
 */
#define TMPMAP_START 1
static int
start_all_aps(void)
{
	u_char mpbiosreason;
	u_int32_t mpbioswarmvec;
	int apic_id, cpu;

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
		bootstacks[cpu] = (char *)malloc(kstack_pages * PAGE_SIZE);

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

}
