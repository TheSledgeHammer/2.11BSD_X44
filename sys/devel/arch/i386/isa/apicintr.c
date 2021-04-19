/*	$OpenBSD: ioapic.c,v 1.41 2018/08/25 16:09:29 kettenis Exp $	*/
/* 	$NetBSD: ioapic.c,v 1.7 2003/07/14 22:32:40 lukem Exp $	*/

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
 * Copyright (c) 1999 Stefan Grefen
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/queue.h>
#include <sys/user.h>

#include <intr.h>
#include <pic.h>
#include <icu.h>

#define	IDTVEC(name)	__CONCAT(X,name)
/* apic interrupt vector table */
extern	IDTVEC(apic_intr0), IDTVEC(apic_intr1), IDTVEC(apic_intr2), IDTVEC(apic_intr3),
		IDTVEC(apic_intr4), IDTVEC(apic_intr5), IDTVEC(apic_intr6), IDTVEC(apic_intr7),
		IDTVEC(apic_intr8), IDTVEC(apic_intr9), IDTVEC(apic_intr10), IDTVEC(apic_intr11),
		IDTVEC(apic_intr12), IDTVEC(apic_intr13), IDTVEC(apic_intr14), IDTVEC(apic_intr15);

#define APIC_INT_VIA_APIC	0x10000000
#define APIC_INT_VIA_MSG	0x20000000
#define APIC_INT_APIC_MASK	0x00ff0000
#define APIC_INT_APIC_SHIFT	16
#define APIC_INT_PIN_MASK	0x0000ff00
#define APIC_INT_PIN_SHIFT	8
#define APIC_INT_LINE_MASK	0x000000ff

#define APIC_IRQ_APIC(x) 	((x & APIC_INT_APIC_MASK) >> APIC_INT_APIC_SHIFT)
#define APIC_IRQ_PIN(x) 	((x & APIC_INT_PIN_MASK) >> APIC_INT_PIN_SHIFT)

struct intrhand *apic_intrhand[256];
int	apic_maxlevel[256];

void
apic_vectors()
{
	/* first icu */
	setidt(32, &IDTVEC(apic_intr0), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(33, &IDTVEC(apic_intr1), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(34, &IDTVEC(apic_intr2), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(35, &IDTVEC(apic_intr3), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(36, &IDTVEC(apic_intr4), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(37, &IDTVEC(apic_intr5), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(38, &IDTVEC(apic_intr6), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(39, &IDTVEC(apic_intr7), 0, SDT_SYS386IGT, SEL_KPL);

	/* second icu */
	setidt(40, &IDTVEC(apic_intr8), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(41, &IDTVEC(apic_intr9), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(42, &IDTVEC(apic_intr10), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(43, &IDTVEC(apic_intr11), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(44, &IDTVEC(apic_intr12), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(45, &IDTVEC(apic_intr13), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(46, &IDTVEC(apic_intr14), 0, SDT_SYS386IGT, SEL_KPL);
	setidt(47, &IDTVEC(apic_intr15), 0, SDT_SYS386IGT, SEL_KPL);
}

/*
 * Caught a stray interrupt, notify
 */
void
apic_strayintr(irq)
	int irq;
{
	static u_long strays;
	struct ioapic_softc *sc;

	strays = APIC_IRQ_APIC(irq);
	sc = ioapic_find(strays);

        /*
         * Stray interrupts on irq 7 occur when an interrupt line is raised
         * and then lowered before the CPU acknowledges it.  This generally
         * means either the device is screwed or something is cli'ing too
         * long and it's timing out.
         */
	if (++strays <= 5)
		log(LOG_ERR, "stray interrupt %d%s\n", irq, strays >= 5 ? "; stopped logging" : "");

	if (sc == NULL) {
			return;
	}
	printf("%s: stray interrupt %d\n", sc->sc_pic->pic_type, irq);
}

void
apic_vectorset(struct ioapic_softc *sc, int pin, int minlevel, int maxlevel)
{
	struct ioapic_intsrc *intpin = &sc->sc_pins[pin];
	int nvector, ovector = intpin->io_vector;

	if (maxlevel == 0) {
		intpin->io_vector = 0;
	}

	apic_intrhand[ovector] = NULL;
}

/*
 * variables: mask, level & depth
 */
void *
apic_intr_establish(int irq, int type, int level, int (*ih_fun)(void *), void *ih_arg, const char *ih_what)
{
	unsigned int ioapic = APIC_IRQ_APIC(irq);
	unsigned int intr = APIC_IRQ_PIN(irq);
	struct ioapic_softc *sc = ioapic_find(ioapic);
	struct ioapic_intsrc *intpin;
	struct intrhand **p, *q, *ih;
	extern int cold;
	int minlevel, maxlevel;

	intpin = &sc->sc_pins[intr];
	switch (intpin->io_type) {
	case IST_NONE:
		intpin->io_type = type;
		break;
	case IST_EDGE:
		intr_shared_edge = 1;
		/* FALLTHROUGH */
	case IST_LEVEL:
		if (type == intpin->io_type)
			break;
	case IST_PULSE:
		if (type != IST_NONE) {
			free(ih, M_DEVBUF, sizeof(*ih));
			return (NULL);
		}
		break;
	}

	if (!cold) {
		ioapic_hwmask(&sc->sc_pic, intr);
	}

	intr_calculatemasks();

	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_flags = flags;
	ih->ih_irq = irq;
	*p = ih;

	/*
	 * Fix up the vector for this pin.
	 * (if cold, defer this until most interrupts have been established,
	 * to avoid too much thrashing of the idt..)
	 */

	if (!ioapic_cold) {
		apic_vectorset(sc, intr, minlevel, maxlevel);
	}

	if (!cold) {
		ioapic_hwunmask(&sc->sc_pic, intr);
	}

	return (ih);
}
