/*	$NetBSD: i8259.c,v 1.4 2003/08/07 16:30:34 agc Exp $	*/

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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/user.h>

#include <machine/cpufunc.h>
#include <machine/intr.h>
#include <machine/pic.h>
#include <machine/apic/i8259.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

struct intrhand 	*intrhand[MAX_INTR_SOURCES];
unsigned i8259_imen;

static void 	i8259_hwmask(struct softpic *, int);
static void 	i8259_hwunmask(struct softpic *, int);
static void 	i8259_setup(struct softpic *, struct cpu_info *, int, int, int);
static void		i8259_reinit_irqs(void);
static void		i8259_register_pic(struct pic *, struct apic *);

struct pic i8259_template = {
		.pic_type = PIC_I8259,
		.pic_hwmask = i8259_hwmask,
		.pic_hwunmask = i8259_hwunmask,
		.pic_addroute = i8259_setup,
		.pic_delroute = i8259_setup,
		.pic_register = i8259_register_pic
};

struct apic i8259_intrmap = {
		.apic_pic_type = PIC_I8259,
		.apic_edge = legacy_stubs,
		.apic_level = legacy_stubs
};

/* initialize 8259's */
void
i8259_default_setup(void)
{
	outb(IO_ICU1, 0x11);				/* reset; program device, four bytes */
	outb(IO_ICU1 + 1, ICU_OFFSET); 		/* starting at this vector index */
	outb(IO_ICU1 + 1, 1 << IRQ_SLAVE); 	/* slave on line 2 */
#ifdef AUTO_EOI_1
	outb(IO_ICU1+1, 2 | 1);				/* auto EOI, 8086 mode */
#else
	outb(IO_ICU1 + 1, 1); 				/* 8086 mode */
#endif
	outb(IO_ICU1 + 1, 0xff); 			/* leave interrupts masked */
	outb(IO_ICU1, 0x68); 				/* special mask mode (if available) */
	outb(IO_ICU1, 0x0a); 				/* Read IRR by default. */
#ifdef REORDER_IRQ
	outb(IO_ICU1, 0xc0 | (3 - 1));		/* pri order 3-7, 0-2 (com2 first) */
#endif

	outb(IO_ICU2, 0x11); 				/* reset; program device, four bytes */
	outb(IO_ICU2 + 1, ICU_OFFSET + 8); 	/* staring at this vector index */
	outb(IO_ICU2 + 1, IRQ_SLAVE);
#ifdef AUTO_EOI_2
	outb(IO_ICU2+1, 2 | 1);				/* auto EOI, 8086 mode */
#else
	outb(IO_ICU2 + 1, 1); 				/* 8086 mode */
#endif
	outb(IO_ICU2 + 1, 0xff);			/* leave interrupts masked */
	outb(IO_ICU2, 0x68); 				/* special mask mode (if available) */
	outb(IO_ICU2, 0x0a); 				/* Read IRR by default. */
}

static void
i8259_hwmask(struct softpic *spic, int pin)
{
	unsigned port;
	u_int8_t byte;
	softpic_pic_hwmask(spic, pin, FALSE, PIC_I8259);

	i8259_imen |= (1 << pin);
#ifdef PIC_MASKDELAY
	delay(10);
#endif
	if (pin > 7) {
		port = IO_ICU2 + 1;
		byte = i8259_imen >> 8;
	} else {
		port = IO_ICU1 + 1;
		byte = i8259_imen & 0xff;
	}
	outb(port, byte);
}

static void
i8259_hwunmask(struct softpic *spic, int pin)
{
	unsigned port;
	u_int8_t byte;
	softpic_pic_hwunmask(spic, pin, FALSE, PIC_I8259);

	disable_intr();
	i8259_imen &= ~(1 << pin);
#ifdef PIC_MASKDELAY
	delay(10);
#endif
	if (pin > 7) {
		port = IO_ICU2 + 1;
		byte = i8259_imen >> 8;
	} else {
		port = IO_ICU1 + 1;
		byte = i8259_imen & 0xff;
	}
	outb(port, byte);
	enable_intr();
}

static void
i8259_setup(struct softpic *spic, struct cpu_info *ci, int pin, int idtvec, int type)
{
	if(cpu_is_primary(ci)) {
		softpic_pic_addroute(spic, ci, pin, idtvec, type, FALSE, PIC_I8259);
		i8259_reinit_irqs();
	}
}

/*
 * Register Local APIC interrupt pins.
 */
static void
i8259_register_pic(pic, apic)
	struct pic *pic;
	struct apic *apic;
{
	pic = &i8259_template;
	apic = &i8259_intrmap;
	softpic_register(pic, apic);
}

static void
i8259_reinit_irqs(void)
{
	int irqs, irq;

	irqs = 0;
	for (irq = 0; irq < NUM_LEGACY_IRQS; irq++) {
		if (intrsrc[irq] != NULL) {
			irqs |= 1 << irq;
		}
	}
	if (irqs >= 0x100) { 			/* any IRQs >= 8 in use */
		irqs |= 1 << IRQ_SLAVE;
	}
	i8259_imen = ~irqs;
	outb(IO_ICU1 + 1, i8259_imen);
	outb(IO_ICU2 + 1, i8259_imen >> 8);
}

void
i8259_reinit(void)
{
	i8259_default_setup();
	i8259_reinit_irqs();
}

unsigned
i8259_setmask(unsigned mask)
{
	unsigned old = i8259_imen;

	i8259_imen = mask;
	outb(IO_ICU1 + 1, i8259_imen);
	outb(IO_ICU2 + 1, i8259_imen >> 8);

	return (old);
}
