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

/* TODO: */
void
softpic_establish_xcall(spic, ci, pin, type, idtvec, isapic, pictemplate)
{
	intr_calculatemasks();

	softpic_apic_allocate(spic, pin, type, idtvec, isapic, pictemplate);
}

struct softpic *
softpic_establish(spic, irq, type, level, isapic, pictemplate)
	struct softpic *spic;
	int irq, type, level, pictemplate;
	bool_t isapic;
{
    struct intrhand     *ih;
    struct intrsource 	*is;
    int idtvec;
    extern int cold;

    softpic_check(spic, irq, isapic, pictemplate);

    ih = malloc(sizeof(*ih), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
    is = malloc(sizeof(*is), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);

    spic->sp_inthnd[irq] = intrhand[irq];
    spic->sp_intsrc[irq] = intrsrc[irq];

    if (ih == NULL || is == NULL) {
        panic("softpic_intr_handler: can't malloc handler info");
    }
    if (!LEGAL_IRQ(irq) || type == IST_NONE) {
        panic("softpic_intr_handler: bogus irq or type");
    }

    if(spic->sp_inthnd != ih) {
    	spic->sp_inthnd = ih;
    }
    if(spic->sp_intsrc != is) {
    	spic->sp_intsrc = is;
    }

    return (spic);
}

/* TODO: irq should equal slot with SMP */
void
softpic_disestablish_xcall(spic, ci, irq, isapic, pictemplate)
	struct softpic *spic;
	struct cpu_info *ci;
	int irq, pictemplate;
	bool_t isapic;
{
    register struct intrhand    *ih, *q, **p;
    register struct intrsource 	*is;
    register struct apic *apic;
    int idtvec;

    ih = intrhand[irq];
    is = intrsrc[irq];
    idtvec = spic->sp_idtvec;

    softpic_pic_hwmask(spic, ih->ih_irq, isapic, pictemplate);

    for (p = &is->is_handlers; (q = *p) != NULL && q != ih; p = &q->ih_next) {
        ;
    }
    if (q) {
        *p = q->ih_next;
    } else {
        panic("softpic_intr_free: handler not registered");
    }

    intr_calculatemasks();

    if(is->is_handlers == NULL) {
    	 softpic_pic_delroute(spic, ci, ih->ih_irq, idtvec, is->is_type, isapic, pictemplate);
    } else if(is->is_count == 0) {
    	 softpic_pic_hwunmask(spic, ih->ih_irq, isapic, pictemplate);
    }
    softpic_apic_free(spic, ih->ih_irq, idtvec, isapic, pictemplate);
    is->is_apic = softpic_handle_apic(spic);
    free(ih, M_DEVBUF);
}


/* TODO: */
void
softpic_disestablish(spic)
	struct softpic *spic;
{

}
