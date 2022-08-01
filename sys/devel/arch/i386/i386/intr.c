/*	$NetBSD: intr.c,v 1.14.2.2 2004/07/02 22:38:26 he Exp $	*/

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
 * Copyright (c) 1993, 1994 Charles Hannum.
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
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/null.h>

#include <arch/i386/include/cpu.h>
#include <arch/i386/include/intr.h>
#include <arch/i386/include/pic.h>

int
intr_allocate_slot_cpu(spic, ci, pin, slot)
    struct softpic *spic;
    struct cpu_info *ci;
    int pin, slot;
{
    struct intrsource *isp;
    struct pic *pic;
    int start, max;

    pic = softpic_handle_pic(spic);
    start = 0;
    max = MAX_INTR_SOURCES;
    slot = -1;

    if (CPU_IS_PRIMARY(ci)) {
    	start = NUM_LEGACY_IRQS;
    }
    for (i = start; i < max; i++) {
        isp = intrsrc[i];
        if (isp != NULL && isp->is_pic == pic && isp->is_pin == pin && pin != -1) {
			slot = i;
			break;
        }
        if (isp == NULL && slot == -1) {
            slot = i;
			continue;
        }
    }
    if (slot == -1) {
		return (EBUSY);
	}
    isp = intrsrc[slot];
	if(isp == NULL) {
		isp = spic->sp_intsrc;
		if (isp == NULL) {
			spic->sp_intsrc[slot] = isp;
			return (ENOMEM);
		}
	}
    spic->sp_slot = slot;
    spic->sp_intsrc = isp;
    return (0);
}

int
intr_allocate_slot(spic, cip, pin, idtvec)
    struct softpic *spic;
    struct cpu_info **cip;
    int pin, idtvec;
{
    CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
    int error, slot;

    ci = &cpu_info;
    slot = spic->sp_slot;
    error = intr_allocate_slot_cpu(spic, ci, pin, slot);
    if (error == 0) {
        goto found;
    }
    for (CPU_INFO_FOREACH(cii, ci)) {
        if (CPU_IS_PRIMARY(ci)) {
            error = intr_allocate_slot_cpu(spic, ci, pin, slot);
            if (error == 0) {
                goto found;
            }
        }
    }
    return (EBUSY);
found:
    if (idtvec == 0) {
		intrsrc[slot] = NULL;
		return (EBUSY);
	}
    spic->sp_idtvec = idtvec;
    spic->sp_slot = slot;
    *cip = ci;
    return (0);
}

void *
intr_establish(isapic, pictemplate, irq, type, level, ih_fun, ih_arg)
	bool_t isapic;
	int irq, type, level, pictemplate;
	int (*ih_fun)(void *);
	void *ih_arg;
{
    register struct softpic *spic;
	register struct intrhand *ih;
    register struct intrsource *is;
    register struct cpu_info *ci;
    int error, idtvec, pin;

    spic = softpic_intr_handler(intrspic, irq, type, isapic, pictemplate);
    ih = spic->sp_inthnd;
    is = spic->sp_intsrc;
    idtvec = idtvector(irq, level, is->is_minlevel, is->is_maxlevel, isapic);
    spic->sp_idtvec = idtvec;
    if(isapic) {
    	ih->ih_irq = spic->sp_pins[irq].sp_irq;
        error = intr_allocate_slot(spic, &ci, ih->ih_irq, idtvec);
        if(error != 0) {
        	free(ih, M_DEVBUF);
        	printf("failed to allocate interrupt slot for PIC pin %d\n", ih->ih_irq);
        	return (NULL);
        }
    }
    error = intr_allocate_slot(spic, &ci, irq, idtvec);
    if(error != 0) {
    	free(ih, M_DEVBUF);
    	printf("failed to allocate interrupt slot for PIC pin %d\n", irq);
    	return (NULL);
    }
    return (ih);
}
