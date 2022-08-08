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

#include <arch/i386/include/pic.h>
#include <arch/i386/include/intr.h>
#include <arch/i386/include/apic/ioapicvar.h>

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

/* place in softpic.c */
struct softpic *
softpic_intr_handler(irq, isapic, pictemplate)
{
	register struct softpic *spic;
	spic = intrspic;
	softpic_check(spic, irq, isapic, pictemplate);

	return (spic);
}

void *
softpic_establish(spic, ci, irq, type, level, ih_fun, ih_arg, slot, idtvec, isapic, pictemplate)
	struct softpic *spic;
	struct cpu_info *ci;
	int irq, type, level, slot, idtvec, pictemplate;
	bool_t isapic;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	register struct intrhand **p, *q, *ih;
	register struct intrsource  *is;
	static struct intrhand fakehand;
	int pin;
	extern int cold;

	KASSERT(ci == curcpu());

    ih = malloc(sizeof(*ih), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
    is = malloc(sizeof(*is), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);

    if (ih == NULL || is == NULL) {
        panic("softpic_establish: can't malloc handler info");
    }
    if (!LEGAL_IRQ(irq) || type == IST_NONE) {
        panic("softpic_establish: bogus irq or type");
    }

	is->is_pin = irq;
	is->is_pic = softpic_handle_pic(spic);
	is->is_apic = softpic_handle_apic(spic);

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
			panic("softpic_establish: irq %d can't share %s with %s", irq,
					intr_typename(intrtype[irq]), intr_typename(type));
			return (NULL);
		}
		break;
	}
	is->is_type = intrtype[irq];

	if(!cold) {
		softpic_pic_hwmask(spic, irq, isapic, pictemplate);
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
    for (p = &intrsrc[slot]->is_handlers; (q = *p) != NULL && q->ih_level > level; p = &q->ih_next) {
        ;
    }


    fakeintr(spic, &fakehand, level);
    *p = &fakehand;

    intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
    ih->ih_pic = softpic_handle_pic(spic);
    ih->ih_apic = softpic_handle_apic(spic);
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_prev = p;
	ih->ih_next = *p;
	ih->ih_level = level;
	ih->ih_irq = irq;
	*p = ih;

	intr_calculatemasks();

	softpic_allocate_apic(spic, irq, type, idtvec, isapic, pictemplate);
	softpic_pic_addroute(spic, ci, irq, idtvec, type, isapic, pictemplate);

    if(!cold) {
		softpic_pic_hwunmask(spic, irq, isapic, pictemplate);
	}

    return (ih);
}

void
softpic_disestablish(spic, ci, irq, type, idtvec, isapic, pictemplate)
	struct softpic *spic;
	struct cpu_info *ci;
	int irq, type, idtvec, pictemplate;
	bool_t isapic;
{
	register struct intrhand    *ih, *q, **p;
    register struct intrsource 	*is;

    KASSERT(ci == curcpu());

    ih = intrhand[irq];
    is = intrsrc[irq];

    if (!LEGAL_IRQ(irq)) {
		panic("softpic_disestablish: bogus irq");
	}

    softpic_pic_hwmask(spic, irq, isapic, pictemplate);

    for (p = &is->is_handlers; (q = *p) != NULL && q != ih; p = &q->ih_next) {
        ;
    }
    if (q) {
        *p = q->ih_next;
    } else {
        panic("softpic_disestablish: handler not registered");
    }

    intr_calculatemasks();

    if(is->is_handlers == NULL) {
    	intrhand[irq] = NULL;
    	intrtype[irq] = IST_NONE;
    	softpic_pic_delroute(spic, ci, irq, idtvec, type, isapic, pictemplate);
    } else if(is->is_count == 0) {
    	softpic_pic_hwunmask(spic, irq, isapic, pictemplate);
    }
    softpic_apic_free(spic, irq, idtvec, isapic, pictemplate);
    free(ih, M_DEVBUF);
}

/* place in intr.c */
void *
intr_establish(irq, type, level, ih_fun, ih_arg, isapic, pictemplate)
	int irq, type, level, pictemplate;
	bool_t isapic;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	register struct softpic *spic;
	register struct intrhand *ih;
	register struct cpu_info *ci;
	int error, pin, slot, idtvec;
	void *arg;

	spic = softpic_intr_handler(irq, isapic, pictemplate);
	if(isapic) {
		pin = spic->sp_pins[irq].sp_irq;
		irq = pin;
	}
	error = intr_allocate_slot(spic, &ci, irq, level &slot, &idtvec, isapic);
	if (error) {
		ih = softpic_establish(spic, ci, irq, type, level, ih_fun, ih_arg, slot, idtvec, isapic, pictemplate);
	} else {
		if (error != 0) {
			free(ih, M_DEVBUF);
			printf("failed to allocate interrupt slot for PIC pin %d\n", irq);
			return (NULL);
		}
	}
	return (ih);
}

void
intr_disestablish(irq, type, isapic, pictemplate)
	int irq, type, pictemplate;
	bool_t isapic;
{
	register struct softpic *spic;
	register struct cpu_info *ci;
	int pin, idtvec;

	spic = softpic_intr_handler(irq, isapic, pictemplate);
	idtvec = spic->sp_idtvec;
	if(isapic) {
		pin = spic->sp_pins[irq].sp_irq;
		irq = pin;
	}
	softpic_disestablish(spic, ci, irq, type, idtvec, isapic, pictemplate);
}
