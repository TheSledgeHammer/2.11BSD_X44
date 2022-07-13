/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <machine/intr.h>
#include <machine/pic.h>
#include <machine/apic/lapicvar.h>
#include <machine/apic/ioapicvar.h>
#include <i386/isa/icu.h>

int  intr_shared_edge;

void *
apic_intr_establish(irq, type, level, ih_fun, ih_arg)
	int irq;
	int type;
	int level;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	struct intrhand **p, *q, *ih;
	static struct intrhand fakehand;// = {apic_fakeintr};
	extern int cold;

	ih = intr_establish(TRUE, PIC_IOAPIC, irq, type, level, ih_fun, ih_arg);
	irq = ih->ih_irq;

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
			free(ih, M_DEVBUF);
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

	if(!cold) {
		softpic_pic_hwmask(intrspic, irq, TRUE, PIC_IOAPIC);
	}
	
	fakeintr(intrspic, &fakehand, level);
	/*
	fakehand.ih_level = level;
	*/
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	*p = ih;

	if(!cold) {
		softpic_pic_hwunmask(intrspic, irq, TRUE, PIC_IOAPIC);
	}

	return (ih);
}

void
apic_intr_disestablish(void *arg)
{
	struct intrhand *ih = arg;

	intr_disestablish(ih);
}

void
apic_intr_string(irqstr, ch, ih)
	char 	*irqstr;
	void 	*ch;
	int	 	ih;
{
	if (ih & APIC_INT_VIA_APIC){
		sprintf(irqstr, "apic %d int %d (irq %d)", APIC_IRQ_APIC(ih), APIC_IRQ_PIN(ih), ih & 0xff);
	} else {
		sprintf(irqstr, "irq %d", ih&0xff);
	}
}
