/*	$NetBSD: ipi.c,v 1.5 2004/02/13 11:36:20 wiz Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <vm/include/vm_extern.h>

#include <arch/i386/include/atomic.h>
#include <arch/i386/include/cpu.h>

#include <arch/i386/apic/apic.h>
#include <arch/i386/apic/lapicreg.h>
#include <arch/i386/apic/lapicvar.h>
#include <devel/sys/smp.h>

void i386_ipi_halt(struct cpu_info *);

#if NNPX > 0
void i386_ipi_synch_fpu(struct cpu_info *);
void i386_ipi_flush_fpu(struct cpu_info *);
#else
#define i386_ipi_synch_fpu 0
#define i386_ipi_flush_fpu 0
#endif

#ifdef MTRR
void i386_ipi_reload_mtrr(struct cpu_info *);
#else
#define i386_ipi_reload_mtrr 0
#endif

void (*ipifunc[I386_NIPI])(struct cpu_info *) = {
		i386_ipi_halt,
		i386_ipi_flush_fpu,
		i386_ipi_synch_fpu,
		i386_ipi_reload_mtrr,
		NULL,
};

int
i386_send_ipi(struct cpu_info *ci, int ipimask)
{
	int ret;

	i386_atomic_setbits_l(&ci->cpu_ipis, ipimask);

	/* Don't send IPI to CPU which isn't (yet) running. */
	if (!(ci->cpu_flags & CPUF_RUNNING))
		return ENOENT;

	ret = i386_ipi(LAPIC_IPI_VECTOR, ci->cpu_apic_id, LAPIC_DLMODE_FIXED);
	if (ret != 0) {
		printf("ipi of %x from %s to %s failed\n",
		    ipimask,
		    curcpu()->cpu_dev->dv_xname,
		    ci->cpu_dev->dv_xname);
	}

	return ret;
}

void
i386_self_ipi(int vector)
{
	lapic_write(LAPIC_ICRLO, vector | LAPIC_DLMODE_FIXED | LAPIC_LVL_ASSERT | LAPIC_DEST_SELF);
}

void
i386_broadcast_ipi(int ipimask)
{
	struct cpu_info *ci, *self = curcpu();
	int count = 0;
	int cii;

	CPU_FOREACH(cii) {
		if(ci == self) {
			continue;
		}
		if ((ci->cpu_flags & CPUF_RUNNING) == 0) {
			continue;
		}
		i386_atomic_setbits_l(&ci->cpu_ipis, ipimask);
		count++;
	}
	if (!count) {
		return;
	}

	lapic_write(LAPIC_ICRLO, LAPIC_IPI_VECTOR | LAPIC_DLMODE_FIXED | LAPIC_LVL_ASSERT | LAPIC_DEST_ALLEXCL);
}

void
i386_multicast_ipi(int cpumask, int ipimask)
{
	struct cpu_info *ci;
	int cii;

	cpumask &= ~(1U << cpu_number());
	if (cpumask == 0) {
		return;
	}

	CPU_FOREACH(cii) {
		if ((cpumask & (1U << ci->cpu_cpuid)) == 0) {
			continue;
		}
		i386_send_ipi(ci, ipimask);
	}
}

void
i386_ipi_handler(void)
{
	struct cpu_info *ci = curcpu();
	u_int32_t pending;
	int bit;

	pending = i386_atomic_testset_ul(&ci->cpu_ipis, 0);

	KDASSERT((pending >> I386_NIPI) == 0);
	while ((bit = ffs(pending)) != 0) {
		bit--;
		pending &= ~(1<<bit);
		ci->cpu_ipi_events[bit].ev_count++;
		(*ipifunc[bit])(ci);
	}
}

/*
 * Common x86 IPI handlers.
 */
void
i386_ipi_halt(struct cpu_info *ci)
{
	disable_intr();
	ci->cpu_flags &= ~CPUF_RUNNING;

	printf("%s: shutting down\n", ci->cpu_dev->dv_xname);
	for (;;) {
		asm volatile("hlt");
	}
}

#if NNPX > 0
void
i386_ipi_flush_fpu(struct cpu_info *ci)
{
	npxsave_cpu(ci, 0);
}

void
i386_ipi_synch_fpu(struct cpu_info *ci)
{
	npxsave_cpu(ci, 1);
}
#endif

#ifdef MTRR
void
i386_ipi_reload_mtrr(struct cpu_info *ci)
{
	if (mem_range_softc.mr_op != NULL)
		mem_range_softc.mr_op->reload(&mem_range_softc);
}
#endif
