/*	$NetBSD: softintr.c,v 1.1 2003/02/26 21:26:12 fvdl Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Generic soft interrupt implementation for 211BSD/i386 & x86.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <machine/pic.h>
#include <machine/intr.h>
#include <machine/softpic.h>

#include <vm/include/vm_extern.h>

static void softintr_register_pic(struct pic *, struct apic *);

/*
 * Generic software interrupt support.
 */
struct pic softintr_pic_template = {
		.pic_type = PIC_SOFT,
		.pic_hwmask = NULL,
		.pic_hwunmask = NULL,
		.pic_addroute = NULL,
		.pic_delroute = NULL,
		.pic_register = softintr_register_pic
};

struct apic softintr_apic_template = {
		.apic_pic_type = PIC_SOFT,
		.apic_edge = NULL,
		.apic_level = NULL
};

static void
softintr_register_pic(pic, apic)
	struct pic *pic;
	struct apic *apic;
{
	pic = &softintr_pic_template;
	apic = &softintr_apic_template;
	softpic_register(pic, apic);
}

struct i386_soft_intr i386_soft_intrs[I386_NSOFTINTR];
const int i386_soft_intr_to_ssir[I386_NSOFTINTR] = {
		SIR_CLOCK,
		SIR_NET,
		SIR_SERIAL,
		SIR_BIO,
};

/*
 * softintr_lock:
 *
 *	Lock software interrupt system.
 */
void
i386_softintr_lock(si, s)
	struct i386_soft_intr *si;
	int s;
{
	s = splhigh();
	simple_lock(si->softintr_slock);
}

/*
 * softintr_unlock:
 *
 *	Unlock software interrupt system.
 */
void
i386_softintr_unlock(si, s)
	struct i386_soft_intr *si;
	int s;
{
	simple_unlock(si->softintr_slock);
	splx(s);
}

/*
 * softintr_init:
 *
 *	Initialize the software interrupt system.
 */
void
softintr_init(void)
{
	struct i386_soft_intr *si;
	int i;

	for (i = 0; i < I386_NSOFTINTR; i++) {
		si = &i386_soft_intrs[i];
		TAILQ_INIT(&si->softintr_q);
		simple_lock_init(si->softintr_slock, "softintr_slock");
		si->softintr_ssir = i386_soft_intr_to_ssir[i];
	}
}

/*
 * softintr_dispatch:
 *
 *	Process pending software interrupts.
 */
void
softintr_dispatch(which)
	int which;
{
	struct i386_soft_intr 		*si;
	struct i386_soft_intrhand 	*sih;
	int s;

	si = &i386_soft_intrs[which];
	for (;;) {
		i386_softintr_lock(si, s);
		sih = TAILQ_FIRST(&si->softintr_q);
		if (sih == NULL) {
			i386_softintr_unlock(si, s);
			break;
		}
		TAILQ_REMOVE(&si->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
		i386_softintr_unlock(si, s);

		(*sih->sih_fn)(sih->sih_arg);
	}
}

/*
 * softintr_establish:		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(level, func, arg)
	int level;
	void (*func)(void *);
	void *arg;
{
	struct i386_soft_intr 		*si;
	struct i386_soft_intrhand 	*sih;
	int which;

	which = (int)intrselect(level);
	if (which == I386_NOINTERRUPT) {
		panic("softintr_establish: no interrupt");
	}
	si = &i386_soft_intrs[which];
	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = si;
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
	}
	return (sih);
}

/*
 * softintr_disestablish:	[interface]
 *
 *	Unregister a software interrupt handler.
 */
void
softintr_disestablish(void *arg)
{
	struct i386_soft_intrhand *sih;
	struct i386_soft_intr *si;
	int s;

	sih = arg;
	si = sih->sih_intrhead;
	i386_softintr_lock(si, s);
	if (sih->sih_pending) {
		TAILQ_REMOVE(&si->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
	}
	i386_softintr_unlock(si, s);

	free(sih, M_DEVBUF);
}

/*
 * softintr_set:	[interface]
 *  Software interrupt update.
 */
void
softintr_set(arg, which)
	void *arg;
	int which;
{
	struct i386_soft_intr *si;
	int sir;

	si = (struct i386_soft_intr *)arg;
	if (si == NULL) {
		switch (which) {
		case I386_SOFTINTR_SOFTBIO:
			sir = SIR_BIO;
			break;
		case I386_SOFTINTR_SOFTCLOCK:
			sir = SIR_CLOCK;
			break;
		case I386_SOFTINTR_SOFTSERIAL:
			sir = SIR_SERIAL;
			break;
		case I386_SOFTINTR_SOFTNET:
			sir = SIR_NET;
			break;
		default:
			sir = I386_NOINTERRUPT;
			panic("softintr_set: no software interrupt");
		}
	} else {
		sir = which;
	}
	softintr(sir);
}

void
softintr_schedule(arg)
	void *arg;
{
	struct i386_soft_intrhand *sih;
	struct i386_soft_intr *si;
	int s;

	sih = (struct i386_soft_intrhand *)arg;
	si = sih->sih_intrhead;
	i386_softintr_lock(si, s);
	if (sih->sih_pending == 0) {
		TAILQ_INSERT_TAIL(&si->softintr_q, sih, sih_q);
		sih->sih_pending = 1;
		softintr_set(si, si->softintr_ssir);
	}
	i386_softintr_unlock(si, s);
}

#ifdef notyet

static void softintr_hwmask(struct softpic *, int);
static void softintr_hwunmask(struct softpic *, int);
static void softintr_addroute(struct softpic *, struct cpu_info *, int, int, int);
static void softintr_delroute(struct softpic *, struct cpu_info *, int, int, int);

struct pic softintr_pic_template = {
		.pic_type = PIC_SOFT,
		.pic_hwmask = softintr_hwmask,
		.pic_hwunmask = softintr_hwunmask,
		.pic_addroute = softintr_addroute,
		.pic_delroute = softintr_delroute,
		.pic_register = softintr_register_pic
};

/* softintr to softpic interface (sisp) */
struct i386_soft_intr *sisp_setup(int, struct softpic *, int, bool_t, int);
struct softpic *sisp_select(struct softpic *, int, bool_t, int);
void sisp_hwmask(struct softpic *, int, bool_t, int);
void sisp_hwunmask(struct softpic *, int, bool_t, int);
void sisp_addroute(struct softpic *, struct cpu_info *, int, int, int, bool_t, int);
void sisp_delroute(struct softpic *, struct cpu_info *, int, int, int, bool_t, int);

static void
softintr_hwmask(spic, pin)
	struct softpic *spic;
	int pin;
{
	if (spic->sp_template != PIC_SOFT) {
		sisp_hwmask(spic, pin, spic->sp_isapic, spic->sp_template);
	}
}

static void
softintr_hwunmask(spic, pin)
	struct softpic *spic;
	int pin;
{
	if (spic->sp_template != PIC_SOFT) {
		sisp_hwunmask(spic, pin, spic->sp_isapic, spic->sp_template);
	}
}

static void
softintr_addroute(spic, ci, pin, idtvec, type)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type;
{
	if (spic->sp_template != PIC_SOFT) {
		sisp_addroute(spic, ci, pin, idtvec, type, spic->sp_isapic, spic->sp_template);
	}
}

static void
softintr_delroute(spic, ci, pin, idtvec, type)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type;
{
	if (spic->sp_template != PIC_SOFT) {
		sisp_delroute(spic, ci, pin, idtvec, type, spic->sp_isapic,
				spic->sp_template);
	}
}

/* softintr to softpic interface (sisp) */
struct i386_soft_intr *
sisp_setup(which, spic, irq, isapic, pictemplate)
	int which;
	struct softpic *spic;
	int irq, pictemplate;
	bool_t isapic;
{
	struct i386_soft_intr *si;
	struct softpic *sisp;

	si = &i386_soft_intrs[which];
	si->softintr_ssir = i386_soft_intr_to_ssir[which];
	softpic_check(spic, irq, isapic, PIC_SOFT);
	if (spic->sp_template == PIC_SOFT) {
		sisp = softpic_intr_handler(irq, isapic, pictemplate);
		si->softintr_softpic = sisp;
		spic->sp_softintr = si;
	}
	return (spic->sp_softintr);
}

struct softpic *
sisp_select(spic, irq, isapic, pictemplate)
	struct softpic *spic;
	int irq, pictemplate;
	bool_t isapic;
{
	struct softintr *si;
	struct softpic *sisp;
	int i;

	si = NULL;
	sisp = NULL;
	for (i = 0; i < I386_NSOFTINTR; i++) {
		si = sisp_setup(i, spic, irq, isapic, pictemplate);
		if (si != NULL) {
			sisp = si->softintr_softpic;
		}
	}
	return (sisp);
}

void
sisp_hwmask(spic, pin, isapic, pictemplate)
	struct softpic *spic;
	int pin, pictemplate;
	bool_t isapic;
{
	register struct softpic *sisp;

	sisp = sisp_select(spic, pin, isapic, pictemplate);
	if (sisp != NULL) {
		switch (sisp->sp_template) {
		case PIC_I8259:
			softpic_pic_hwmask(sisp, pin, FALSE, PIC_I8259);
			break;
		case PIC_IOAPIC:
			softpic_pic_hwmask(sisp, pin, TRUE, PIC_IOAPIC);
			break;
		case PIC_LAPIC:
			softpic_pic_hwmask(sisp, pin, TRUE, PIC_LAPIC);
			break;
		case PIC_SOFT:
			//printf("softintr_hwmask: cannot return self");
			panic("softintr_hwmask: infinite loop");
			break;
		}
	}
}

void
sisp_hwunmask(spic, pin, isapic, pictemplate)
	struct softpic *spic;
	int pin, pictemplate;
	bool_t 	isapic;
{
	register struct softpic *sisp;

	sisp = sisp_select(spic, pin, isapic, pictemplate);
	if (sisp != NULL) {
		switch (sisp->sp_template) {
		case PIC_I8259:
			softpic_pic_hwunmask(sisp, pin, FALSE, PIC_I8259);
			break;
		case PIC_IOAPIC:
			softpic_pic_hwunmask(sisp, pin, TRUE, PIC_IOAPIC);
			break;
		case PIC_LAPIC:
			softpic_pic_hwunmask(sisp, pin, TRUE, PIC_LAPIC);
			break;
		case PIC_SOFT:
			//printf("softintr_hwunmask: cannot return self");
			panic("softintr_hwunmask: infinite loop");
			break;
		}
	}
}

void
sisp_addroute(spic, ci, pin, idtvec, type, isapic, pictemplate)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type, pictemplate;
	bool_t  isapic;
{
	register struct softpic *sisp;

	sisp = sisp_select(spic, pin, isapic, pictemplate);
	if (sisp != NULL) {
		switch (sisp->sp_template) {
		case PIC_I8259:
			softpic_pic_addroute(sisp, ci, pin, idtvec, type, FALSE, PIC_I8259);
			break;
		case PIC_IOAPIC:
			softpic_pic_addroute(sisp, ci, pin, idtvec, type, TRUE, PIC_IOAPIC);
			break;
		case PIC_LAPIC:
			softpic_pic_addroute(sisp, ci, pin, idtvec, type, TRUE, PIC_LAPIC);
			break;
		case PIC_SOFT:
			//printf("softintr_addroute: cannot return self");
			panic("softintr_addroute: infinite loop");
			break;
		}
	}
}

void
sisp_delroute(spic, ci, pin, idtvec, type, isapic, pictemplate)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type, pictemplate;
	bool_t isapic;
{
	register struct softpic *sisp;

	sisp = sisp_select(spic, pin, isapic, pictemplate);
	if (sisp != NULL) {
		switch (sisp->sp_template) {
		case PIC_I8259:
			softpic_pic_delroute(sisp, ci, pin, idtvec, type, FALSE, PIC_I8259);
			break;
		case PIC_IOAPIC:
			softpic_pic_delroute(sisp, ci, pin, idtvec, type, TRUE, PIC_IOAPIC);
			break;
		case PIC_LAPIC:
			softpic_pic_delroute(sisp, ci, pin, idtvec, type, TRUE, PIC_LAPIC);
			break;
		case PIC_SOFT:
			//printf("softintr_delroute: cannot return self");
			panic("softintr_delroute: infinite loop");
			break;
		}
	}
}

#endif /* notyet */
