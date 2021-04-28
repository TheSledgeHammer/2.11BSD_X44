/*	$NetBSD: x86_softintr.c,v 1.3 2020/05/08 21:43:54 ad Exp $	*/

/*
 * Copyright (c) 2007, 2008, 2009, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran, and by Jason R. Thorpe.
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
 * Copyright 2002 (c) Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/user.h>

#include <devel/arch/i386/isa/icu.h>
#include <arch/i386/include/intr.h>
#include <arch/i386/include/pic.h>
#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

struct intrsource 	*intrsrc[MAX_INTR_SOURCES];
struct intrhand 	*intrhand[MAX_INTR_SOURCES];

int 				intrtype[MAX_INTR_SOURCES];
int 				intrmask[MAX_INTR_SOURCES];
int 				intrlevel[MAX_INTR_SOURCES];
int 				intr_shared_edge;

struct intr_softc {
	struct intrsource 	*sc_isrc;
	struct intrhand	 	*sc_ihand;
	struct softpic		*sc_softpic;

	int					sc_itype;
	int					sc_imask;
	int					sc_ilevel;
};

struct softpic {
	TAILQ_HEAD(, pic)	pic_head;
	int					pic_pin;
};

struct pic {
	TAILQ_ENTRY(pic)	pic_entry;
};


struct intrsource *
intrsource_create(struct pic *pic, int pin)
{
	struct intrsource *isrc;

	isrc = &intrsrc[pin];
	isrc->is_pic = pic;
	isrc->is_pin = pin;

	return (isrc);
}

struct intrhand *
intrhand_create(struct pic *pic, int irq)
{
	struct intrhand *ihnd;

	ihnd = &intrhand[irq];
	ihnd->ih_pic = pic;
	ihnd->ih_irq = irq;

	return (ihnd);
}

struct ioapic_intsrc *
intr_pic_create(irq)
	int irq;
{
	struct ioapic_softc  *sc;
	struct ioapic_intsrc *intpin;
	u_int apicid;
	u_int pin;

	apicid = APIC_IRQ_APIC(irq);
	pin	= APIC_IRQ_PIN(irq);
	sc = ioapic_find(apicid);

	intpin = &sc->sc_pins[pin];
	return (intpin);
}

void
intr_io(pictemp, pin, irq)
	int pictemp, pin, irq;
{
	struct pic 				*pic;
	struct intrsource 		*isrc;
	struct intrhand 		*ihnd;

	pic = intr_handle_pic(pictemp);
	if(pic) {
		isrc = intrsource_create(pic, pin);
		ihnd = intrhand_create(pic, irq);
		isrc->is_handlers = ihnd;
	}
}

void
intr_legacy_vectors()
{
	int i;
	for(i = 0; i < NUM_LEGACY_IRQS; i++) {
		int idx = ICU_OFFSET + i;
		setidt(idx, &IDTVEC(legacy), 0, SDT_SYS386IGT, SEL_KPL);
	}
}

void
intr_apic_vectors()
{
	int idx;
	for(idx = 0; idx < MAX_INTR_SOURCES; idx++) {
		setidt(idx, &IDTVEC(apic), 0, SDT_SYS386IGT, SEL_KPL);
	}
}

void
intr_x2apic_vectors()
{
	int idx;
	for(idx = 0; idx < MAX_INTR_SOURCES; idx++) {
		setidt(idx, &IDTVEC(x2apic), 0, SDT_SYS386IGT, SEL_KPL);
	}
}

void
intr_calculatemasks()
{
	int irq, level, unusedirqs;
	struct intrhand **p, *q;

	/* First, figure out which levels each IRQ uses. */
	unusedirqs = 0xffffffff;
	for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
		int levels = 0;
		if(intrsrc[irq] == NULL) {
			intrlevel[irq] = 0;
			continue;
		}

		for (p = intrhand[irq]; (q = *p) != NULL; p = &q->ih_next) {
			levels |= 1U << q->ih_level;
		}
		intrlevel[irq] = levels;
		if (levels) {
			unusedirqs &= ~(1U << irq);
		}
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		int irqs = 0;
		for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
			if (intrlevel[irq] & (1U << level)) {
				irqs |= 1U << irq;
			}
		}
		imask[level] = irqs | unusedirqs;
	}

	/* Initialize soft interrupt masks */
	init_intrmask();

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	for (level = 0; level < (NIPL-1); level++) {
		imask[level+1] |= imask[level];
	}

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
		int irqs = 1 << irq;
		int maxlevel = IPL_NONE;
		int minlevel = IPL_HIGH;

		if(intrsrc[irq] != NULL) {
			continue;
		};

		for (q = intrsrc[irq]->is_handlers; q; q = q->ih_next) {
			irqs |= imask[q->ih_level];
			if (q->ih_level < minlevel) {
				minlevel = q->ih_level;
			}
			if (q->ih_level > maxlevel) {
				maxlevel = q->ih_level;
			}
		}
		intrmask[irq] = irqs;
		intrsrc[irq]->is_maxlevel =  maxlevel;
		intrsrc[irq]->is_minlevel = minlevel;
	}

	/* Lastly, determine which IRQs are actually in use. */
	{
		int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++) {
			if (intrhand[irq]) {
				irqs |= 1 << irq;
			}
		}
		if (irqs >= 0x100) { /* any IRQs >= 8 in use */
			irqs |= 1 << IRQ_SLAVE;
		}
		imen = ~irqs;
		outb(IO_ICU1 + 1, imen);
		outb(IO_ICU2 + 1, imen >> 8);
	}

	for (level = 0; level < NIPL; level++) {
		iunmask[level] = ~imask[level];
	}
}

/*
 * soft interrupt masks
 */
void
init_intrmask()
{
	/*
	 * Initialize soft interrupt masks to block themselves.
	 */
	SOFTINTR_MASK(imask[IPL_SOFTCLOCK], SIR_CLOCK);
	SOFTINTR_MASK(imask[IPL_SOFTNET], SIR_NET);
	SOFTINTR_MASK(imask[IPL_SOFTSERIAL], SIR_SERIAL);

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_NONE];
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	imask[IPL_IMP] |= imask[IPL_TTY];

	imask[IPL_AUDIO] |= imask[IPL_IMP];

	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and disk drivers, clock > imp.
	 */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];

	/*
	 * IPL_HIGH must block everything that can manipulate a run queue.
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];

	/*
	 * We need serial drivers to run at the absolute highest priority to
	 * avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];
}

int
fakeintr(arg)
	void *arg;
{
	return 0;
}

void *
intr_establish(irq, type, level, ih_fun, ih_arg)
	int irq;
	int type;
	int level;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	struct intrhand **p, *q, *ih;
	static struct intrhand fakehand = { fakeintr };
	extern int cold;

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL) {
		panic("intr_establish: can't malloc handler info");
	}
	if (!LEGAL_IRQ(irq) || type == IST_NONE) {
		panic("intr_establish: bogus irq or type");
	}

	switch (intrtype[irq]) {
	case IST_NONE:
		intrtype[irq] = type;
		break;
	case IST_EDGE:
		intr_shared_edge = 1;
		/* FALLTHROUGH */
	case IST_LEVEL:
		if (intrtype[irq] == type) {
			break;
		}
		/* FALLTHROUGH */
	case IST_PULSE:
		if (type != IST_NONE) {
			panic("intr_establish: can't share %s with %s",
			    intr_typename(intrtype[irq]),
			    intr_typename(type));
			free(ih, M_DEVBUF, sizeof(*ih));
			return (NULL);
		}
		break;
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &intrhand[irq]; (q = *p) != NULL; p = &q->ih_next) {
		;
	}

	fakehand.ih_level = level;
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_count = 0;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	*p = ih;

	return (ih);
}

void
intr_disestablish(ih)
	struct intrhand *ih;
{
	int irq = ih->ih_irq;
	struct intrhand **p, *q;

	if (!LEGAL_IRQ(irq)) {
		panic("intr_disestablish: bogus irq");
	}
	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &intrhand[irq]; (q = *p) != NULL && q != ih; p = &q->ih_next) {
		;
	}

	if (q) {
		*p = q->ih_next;
	} else {
		panic("intr_disestablish: handler not registered");
	}
	free(ih, M_DEVBUF);

	intr_calculatemasks();

	if (intrhand[irq] == NULL) {
		intrtype[irq] = IST_NONE;
	}
}

char *
intr_typename(type)
	int type;
{
	switch (type) {
	case IST_NONE:
		return ("none");
	case IST_PULSE:
		return ("pulsed");
	case IST_EDGE:
		return ("edge-triggered");
	case IST_LEVEL:
		return ("level-triggered");
	default:
		panic("intr_typename: invalid type %d", type);
	}
}

int
intr_allocate_slot(pic, irq, pin, level, cip, idx, idt_slot)
	struct pic  *pic;
	struct cpu_info *cip;
	int irq, pin, level;
	int *idx, *idt_slot;
{
	struct cpu_info *ci;
	struct intrsource *isp;
	int slot, idtvec, error;

	if (irq != -1) {
		ci = &cpu_info;
		for (slot = 0 ; slot < MAX_INTR_SOURCES ; slot++) {
			isp = &intrsrc[slot];
			if (isp != NULL && isp->is_pic == pic && isp->is_pin == pin ) {
				goto duplicate;
			}
		}
		slot = irq;
		isp = &intrsrc[slot];
		if (isp == NULL) {
			MALLOC(isp, struct intrsource *, sizeof(struct intrsource), M_DEVBUF, M_NOWAIT|M_ZERO);
			if (isp == NULL) {
				return ENOMEM;
			}
			&intrsrc[slot] = isp;
		} else {
			if (isp->is_pin != pin) {
				if (pic == &i8259_template) {
					return EINVAL;
				}
				goto other;
			}
		}
duplicate:
		if (pic == &i8259_template) {
			idtvec = ICU_OFFSET + irq;
		} else {
			if (level > isp->is_maxlevel) {

			}
			if (isp->is_minlevel == 0 || level < isp->is_minlevel) {
				idtvec = idt_vec_alloc(APIC_LEVEL(level), IDT_INTR_HIGH);
				if (idtvec == 0) {
					return (EBUSY);
				}
			} else {
				idtvec = isp->is_idtvec;
			}
		}
	} else {
other:
	ci = &cpu_info;
	error = intr_allocate_slot_cpu(ci, pic, pin, &slot);
	if (error == 0) {
		goto found;
	}


	*idt_slot = idtvec;
	*idx = slot;
	*cip = ci;
	return (0);
}
