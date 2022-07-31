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

#include <machine/pic.h>
#include <machine/intr.h>
#include <machine/apic/ioapicvar.h>

struct softpic 					 	*intrspic;
static TAILQ_HEAD(pic_list, pic) 	pichead;
static TAILQ_HEAD(apic_list, apic) 	apichead;

int	softpic_register_pic(struct pic *);
int	softpic_register_apic(struct apic *);

/* Softpic Methods common to pic & apic*/
void
softpic_init(void)
{
	struct softpic *spic;
	spic = intrspic;
	if(spic == NULL) {
		spic = (struct softpic *)malloc(sizeof(struct softpic *), M_DEVBUF, M_WAITOK);
	}
	TAILQ_INIT(&pichead);
	TAILQ_INIT(&apichead);

	spic->sp_intsrc = (struct intrsource *)intrsrc;
	spic->sp_inthnd = (struct intrhand *)intrhand;
}

void
softpic_check(spic, irq, isapic, pictemplate)
    struct softpic  *spic;
    int             irq, pictemplate;
    bool_t 			isapic;
{
    int apicid = APIC_IRQ_APIC(irq);
    int pin = APIC_IRQ_PIN(irq);

	if (spic == intrspic) {
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
	if(softpic_register_pic(pic) != 0 || pic == NULL) {
		printf("softpic_register: pic not found!\n");
	} else {
		printf("softpic_register: pic registered\n");
	}
	if(softpic_register_apic(apic) != 0 || apic == NULL) {
		printf("softpic_register: apic not found!\n");
	} else {
		printf("softpic_register: apic registered\n");
	}
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

	switch(spic->sp_template) {
	case PIC_I8259:
		pic = softpic_lookup_pic(PIC_I8259);
		if(pic == &i8259_template && spic->sp_template == pic->pic_type) {
			return (pic);
		}
		break;
	case PIC_IOAPIC:
		pic = softpic_lookup_pic(PIC_IOAPIC);
		if(pic == &ioapic_template && spic->sp_template == pic->pic_type) {
			return (pic);
		}
		break;
	case PIC_LAPIC:
		pic = softpic_lookup_pic(PIC_LAPIC);
		if(pic == &lapic_template && spic->sp_template == pic->pic_type) {
			return (pic);
		}
		break;
	case PIC_SOFT:
		pic = softpic_lookup_pic(PIC_SOFT);
		if(pic == &softintr_template && spic->sp_template == pic->pic_type) {
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

struct softpic *
softpic_intr_handler(spic, irq, type, isapic, pictemplate)
	struct softpic *spic;
	bool_t isapic;
	int irq, type, pictemplate;
{
	struct intrhand *ih;
	extern int cold;

	softpic_check(spic, irq, isapic, pictemplate);

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL) {
		panic("softpic_intr_handler: can't malloc handler info");
	}
	if (!LEGAL_IRQ(irq) || type == IST_NONE) {
		panic("softpic_intr_handler: bogus irq or type");
	}

	spic->sp_inthnd = ih;

	return (spic);
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
        error = 1;
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
    switch(spic->sp_template) {
        case PIC_I8259:
            apic = softpic_lookup_apic(PIC_I8259);
            if(apic == &legacy_intrmap && spic->sp_template == apic->apic_pic_type) {
                return (apic);
            }
            break;
        case PIC_IOAPIC:
            apic = softpic_lookup_apic(PIC_IOAPIC);
            if(apic == &ioapic_intrmap && spic->sp_template == apic->apic_pic_type) {
                return (apic);
            }
            break;
        case PIC_LAPIC:
            apic = softpic_lookup_apic(PIC_LAPIC);
            if(apic == &lapic_intrmap && spic->sp_template == apic->apic_pic_type) {
                return (apic);
            }
            break;
    }
    return (NULL);
}
