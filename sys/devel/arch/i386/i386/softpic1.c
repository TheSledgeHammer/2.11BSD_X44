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

struct softpic *
softpic_intr_alloc(spic, irq, type, level, isapic, pictemplate)
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
    if (ih == NULL || is == NULL) {
        panic("softpic_intr_handler: can't malloc handler info");
    }
    if (!LEGAL_IRQ(irq) || type == IST_NONE) {
        panic("softpic_intr_handler: bogus irq or type");
    }

    intr_calculatemasks();
    idtvec = idtvector(irq, level, is->is_minlevel, is->is_maxlevel, isapic);
    spic->sp_idtvec = idtvec;
    if(spic->sp_inthnd != ih) {
    	spic->sp_inthnd = ih;
    }
    if(spic->sp_intsrc != is) {
    	spic->sp_intsrc = is;
    }

    return (spic);
}

struct softpic *
softpic_intr_free(spic, irq, type, level, isapic, pictemplate)
	struct softpic *spic;
	int irq, type, level, pictemplate;
	bool_t isapic;
{
    register struct intrhand    *ih, *q, **p;
    register struct intrsource 	*is;
    int idtvec;

    softpic_check(spic, irq, isapic, pictemplate);

    ih = intrhand[irq];
    is = intrsrc[irq];
    idtvec = idtvector(irq, level, is->is_minlevel, is->is_maxlevel, isapic);

    is->is_handlers = ih;
    for (p = &ih; (q = *p) != NULL && q != spic->sp_inthnd; p = &q->ih_next) {
        ;
    }
    if (q) {
        *p = q->ih_next;
    } else {
        panic("softpic_intr_free: handler not registered");
    }

	free(ih, M_DEVBUF);
    free(is, M_DEVBUF);

    intr_calculatemasks();

    if(ih == NULL) {
        is->is_handlers = NULL;
        type = IST_NONE;
    }
    is = NULL;
    if(isapic == FALSE) {
        idt_vec_free(idtvec);
    }
    is->is_apic->apic_resume = NULL;
	is->is_apic->apic_recurse = NULL;
    if(spic->sp_idtvec != idtvec) {
        spic->sp_idtvec = idtvec;
    }
    if(spic->sp_inthnd != ih) {
        spic->sp_inthnd = ih;
    }
    if(spic->sp_intsrc != is) {
        spic->sp_intsrc = is;
    }
    return (spic);
}

void *
intr_establish(irq, type, level, ih_fun, ih_arg, isapic, pictemplate)
{
    register struct softpic  *spic;

    spic = softpic_intr_alloc(intrspic, irq, type, isapic, pictemplate);

    return (spic->sp_inthnd);
}

void *
intr_disestablish(irq, type, level, isapic, pictemplate)
{
    register struct softpic  *spic;

    spic = softpic_intr_free(intrspic, irq, type, level, isapic, pictemplate);

    return (spic->sp_inthnd);
}
