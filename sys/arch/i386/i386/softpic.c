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

#include <machine/gdt.h>
#include <machine/pic.h>
#include <machine/intr.h>
#include <machine/apic/ioapicvar.h>

static struct softpic 			intrspic;
static TAILQ_HEAD(pic_list, pic) 	pichead;
static TAILQ_HEAD(apic_list, apic) 	apichead;

int  intr_shared_edge;

int	softpic_register_pic(struct pic *);
int	softpic_register_apic(struct apic *);

/*
 * Common Softpic Methods
 */
void
softpic_init(void)
{
	struct softpic *spic;
	
	spic = &intrspic;
	if (spic == NULL) {
		spic = (struct softpic*) malloc(sizeof(struct softpic*), M_DEVBUF,
		M_WAITOK);
	}
	TAILQ_INIT(&pichead);
	TAILQ_INIT(&apichead);

	spic->sp_intsrc = (struct intrsource*) intrsrc;
	spic->sp_inthnd = (struct intrhand*) intrhand;
}

void
softpic_check(spic, irq, isapic, pictemplate)
    struct softpic  *spic;
    int             irq, pictemplate;
    bool_t 			isapic;
{
    int apicid = APIC_IRQ_APIC(irq);
    int pin = APIC_IRQ_PIN(irq);

	if (spic == &intrspic) {
		spic->sp_template = pictemplate;
		if (isapic == TRUE) {
			spic->sp_isapic = TRUE;
			spic->sp_pins[pin].sp_apicid = apicid;
			spic->sp_pins[pin].sp_irq = pin;
			spic->sp_pins[pin].sp_pin = pin;
		} else {
			spic->sp_isapic = FALSE;
			spic->sp_pins[irq].sp_irq = irq;
			spic->sp_pins[irq].sp_pin = irq;
		}
	} else {
		return;
	}
}

void
softpic_register(pic, apic)
	struct pic 	*pic;
	struct apic *apic;
{
	int error;
	
	if (softpic_register_pic(pic) != 0 || pic == NULL) {
		printf("softpic_register: pic not found!\n");
	} else {
		printf("softpic_register: pic registered\n");
	}
	if (softpic_register_apic(apic) != 0 || apic == NULL) {
		printf("softpic_register: apic not found!\n");
	} else {
		printf("softpic_register: apic registered\n");
	}
}

struct softpic *
softpic_intr_handler(irq, isapic, pictemplate)
	int irq, pictemplate;
	bool_t isapic;
{
	register struct softpic *spic;
	
	spic = &intrspic;
	softpic_check(spic, irq, isapic, pictemplate);

	return (spic);
}

/*
 * PIC Methods
 */
static int
softpic_pic_registered(pic)
	struct pic *pic;
{
	struct pic *p;

	TAILQ_FOREACH(p, &pichead, pic_entry) {
		if (p == pic) {
			return (1);
		}
	}
	return (0);
}

int
softpic_register_pic(pic)
	struct pic *pic;
{
	int error;

	if (softpic_pic_registered(pic)) {
		error = EBUSY;
	} else {
		TAILQ_INSERT_TAIL(&pichead, pic, pic_entry);
		error = 0;
	}

	return (error);
}

static struct pic *
softpic_lookup_pic(pictemplate)
	int pictemplate;
{
	struct pic *pic;
	
	TAILQ_FOREACH(pic, &pichead, pic_entry) {
		if(pic->pic_type == pictemplate) {
			return (pic);
		}
	}
	return (NULL);
}

struct pic *
softpic_handle_pic(spic)
	struct softpic *spic;
{
	struct pic *pic;

	switch (spic->sp_template) {
	case PIC_I8259:
		pic = softpic_lookup_pic(PIC_I8259);
		if (pic == &i8259_pic_template && spic->sp_template == pic->pic_type) {
			return (pic);
		}
		break;
	case PIC_IOAPIC:
		pic = softpic_lookup_pic(PIC_IOAPIC);
		if (pic == &ioapic_pic_template && spic->sp_template == pic->pic_type) {
			return (pic);
		}
		break;
	case PIC_LAPIC:
		pic = softpic_lookup_pic(PIC_LAPIC);
		if (pic == &lapic_pic_template && spic->sp_template == pic->pic_type) {
			return (pic);
		}
		break;
	case PIC_SOFT:
		pic = softpic_lookup_pic(PIC_SOFT);
		if (pic == &softintr_pic_template
				&& spic->sp_template == pic->pic_type) {
			return (pic);
		}
		break;
	}
	return (NULL);
}

void
softpic_pic_hwmask(spic, pin, isapic, pictemplate)
	struct softpic *spic;
	int pin, pictemplate;
	bool_t isapic;
{
	register struct pic *pic;
	
	softpic_check(spic, pin, isapic, pictemplate);
	spic->sp_intsrc->is_pic = softpic_handle_pic(spic);
	pic = spic->sp_intsrc->is_pic;
	if (pic != NULL) {
		(*pic->pic_hwmask)(spic, pin);
	}
}

void
softpic_pic_hwunmask(spic, pin, isapic, pictemplate)
	struct softpic *spic;
	int pin, pictemplate;
	bool_t 	isapic;
{
	register struct pic *pic;
	
	softpic_check(spic, pin, isapic, pictemplate);
	spic->sp_intsrc->is_pic = softpic_handle_pic(spic);
	pic = spic->sp_intsrc->is_pic;
	if (pic != NULL) {
		(*pic->pic_hwunmask)(spic, pin);
	}
}

void
softpic_pic_addroute(spic, ci, pin, idtvec, type, isapic, pictemplate)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type, pictemplate;
	bool_t  isapic;
{
	register struct pic *pic;
	
	softpic_check(spic, pin, isapic, pictemplate);
	spic->sp_intsrc->is_pic = softpic_handle_pic(spic);
	pic = spic->sp_intsrc->is_pic;
	if (pic != NULL) {
		(*pic->pic_addroute)(spic, ci, pin, idtvec, type);
	}
}

void
softpic_pic_delroute(spic, ci, pin, idtvec, type, isapic, pictemplate)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type, pictemplate;
	bool_t isapic;
{
	register struct pic *pic;
	
	softpic_check(spic, pin, isapic, pictemplate);
	spic->sp_intsrc->is_pic = softpic_handle_pic(spic);
	pic = spic->sp_intsrc->is_pic;
	if (pic != NULL) {
		(*pic->pic_delroute)(spic, ci, pin, idtvec, type);
	}
}

/*
 * APIC Methods
 */
static int
softpic_apic_registered(apic)
    struct apic *apic;
{
    struct apic *p;

    TAILQ_FOREACH(p, &apichead, apic_entry) {
        if (p == apic) {
            return (1);
        }
    }
    return (0);
}

int
softpic_register_apic(apic)
        struct apic *apic;
{
    int error;

    if (softpic_apic_registered(apic)) {
        error = EBUSY;
    } else {
        TAILQ_INSERT_TAIL(&apichead, apic, apic_entry);
        error = 0;
    }
    return (error);
}

static struct apic *
softpic_lookup_apic(pictemplate)
    int             pictemplate;
{
    struct apic *apic;
	
    TAILQ_FOREACH(apic, &apichead, apic_entry) {
        if(apic->apic_pic_type == pictemplate) {
            return (apic);
        }
    }
    return (NULL);
}

struct apic *
softpic_handle_apic(spic)
    struct softpic *spic;
{
    struct apic *apic;
	
    switch (spic->sp_template) {
	case PIC_I8259:
		apic = softpic_lookup_apic(PIC_I8259);
		if (apic == &i8259_apic_template
				&& spic->sp_template == apic->apic_pic_type) {
			return (apic);
		}
		break;
	case PIC_IOAPIC:
		apic = softpic_lookup_apic(PIC_IOAPIC);
		if (apic == &ioapic_apic_template
				&& spic->sp_template == apic->apic_pic_type) {
			return (apic);
		}
		break;
	case PIC_LAPIC:
		apic = softpic_lookup_apic(PIC_LAPIC);
		if (apic == &lapic_apic_template
				&& spic->sp_template == apic->apic_pic_type) {
			return (apic);
		}
		break;
	case PIC_SOFT:
		apic = softpic_lookup_apic(PIC_SOFT);
		if (apic == &softintr_apic_template
				&& spic->sp_template == apic->apic_pic_type) {
			return (apic);
		}
		break;
	}
	return (NULL);
}

void
softpic_apic_allocate(spic, pin, type, idtvec, isapic, pictemplate)
	struct softpic *spic;
	int pin, type, idtvec, pictemplate;
	bool_t isapic;
{
	register struct apic *apic;
	register struct intrstub *stubp;

	softpic_check(spic, pin, isapic, pictemplate);
	spic->sp_intsrc->is_apic = softpic_handle_apic(spic);
	apic = spic->sp_intsrc->is_apic;
	if (apic != NULL) {
		if (apic->apic_resume == NULL || spic->sp_idtvec != idtvec) {
			if (spic->sp_idtvec != 0 && spic->sp_idtvec != idtvec) {
				idt_vec_free(idtvec);
			}
			spic->sp_idtvec = idtvec;
			switch (type) {
			case IST_EDGE:
				stubp = &apic->apic_edge[pin];
				break;
			case IST_LEVEL:
				stubp = &apic->apic_level[pin];
				break;
			}
			apic->apic_resume = stubp->ist_resume;
			apic->apic_recurse = stubp->ist_recurse;
			idt_vec_set(idtvec, stubp->ist_entry);
		}
	}
}

void
softpic_apic_free(spic, pin, idtvec, isapic, pictemplate)
	struct softpic *spic;
	int pin, idtvec, pictemplate;
	bool_t isapic;
{
	register struct apic *apic;

	softpic_check(spic, pin, isapic, pictemplate);
	spic->sp_intsrc->is_apic = softpic_handle_apic(spic);
	apic = spic->sp_intsrc->is_apic;
	if (apic != NULL) {
		if(spic->sp_intsrc[pin].is_handlers != NULL) {
			return;
		}
		spic->sp_intsrc = NULL;
		if(isapic) {
			idt_vec_free(idtvec);
			apic->apic_resume = NULL;
			apic->apic_recurse = NULL;
		}
	}
}

/*
 * Softpic Interrupt methods
 */

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

	if (!cold) {
		softpic_pic_hwmask(spic, irq, isapic, pictemplate);
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &intrsrc[slot]->is_handlers;
			(q = *p) != NULL && q->ih_level > level; p = &q->ih_next) {
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
	ih->ih_next = *p;
	ih->ih_level = level;
	ih->ih_irq = irq;
	ih->ih_slot = slot;
	*p = ih;

	if (spic->sp_slot != slot) {
		spic->sp_slot = slot;
	}

	intr_calculatemasks();

	softpic_apic_allocate(spic, irq, type, idtvec, isapic, pictemplate);
	softpic_pic_addroute(spic, ci, irq, idtvec, type, isapic, pictemplate);

	if (!cold) {
		softpic_pic_hwunmask(spic, irq, isapic, pictemplate);
	}

	return (ih);
}

void
softpic_disestablish(spic, ci, irq, idtvec, isapic, pictemplate)
	struct softpic *spic;
	struct cpu_info *ci;
	int irq, idtvec, pictemplate;
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

	if (is->is_handlers == NULL) {
		intrhand[irq] = NULL;
		intrtype[irq] = IST_NONE;
		softpic_pic_delroute(spic, ci, irq, idtvec, is->is_type, isapic,
				pictemplate);
	} else if (is->is_count == 0) {
		softpic_pic_hwunmask(spic, irq, isapic, pictemplate);
	}
	softpic_apic_free(spic, irq, idtvec, isapic, pictemplate);
	free(ih, M_DEVBUF);
}
