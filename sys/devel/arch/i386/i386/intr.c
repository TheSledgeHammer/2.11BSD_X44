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

#include <arch/i386/include/cpu.h>
#include <arch/i386/include/intr.h>
#include <arch/i386/include/pic.h>

int
intr_allocate_slot_cpu(spic, ci, pin, index)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, *index;
{
	struct intrsource *isp;
	struct pic *pic;
	int start, slot, i;

	start = CPU_IS_PRIMARY(ci) ? NUM_LEGACY_IRQS : 0;
	slot = -1;

	pic = softpic_handle_pic(spic);
	for (i = start; i < MAX_INTR_SOURCES ; i++) {
		isp = intrsrc[slot];
		if (isp != NULL && isp->is_pic == pic && isp->is_pin == pin) {
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
			return (ENOMEM);
		}
	}
	*index = slot;
	return (0);
}

/* TODO: XXXX */
int
intr_allocate_slot(cip, pin, level, type, isapic, pictemplate, index, idt_slot)
	struct cpu_info **cip;
	int pin, level, type, pictemplate, *index, *idt_slot;
	bool_t isapic;
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	struct softpic *spic;
	struct intrsource *isp;
	struct pic *pic;
	int slot, idtvec, legacy_irq, error;

	spic = softpic_intr_handler(intrspic, pin, type, isapic, pictemplate);
	pic = softpic_handle_pic(spic);
	if(pictemplate == PIC_I8259) {
		legacy_irq = spic->sp_pins[pin].sp_irq;
		idtvec = ICU_OFFSET + legacy_irq;
	}

	if(legacy_irq != -1) {
		for (slot = 0 ; slot < MAX_INTR_SOURCES ; slot++) {
			isp = intrsrc[slot];
			if (isp != NULL && isp->is_pic == pic && isp->is_pin == pin) {
				goto duplicate;
			}
		}
		slot = spic->sp_pins[pin].sp_pin;
		isp = intrsrc[slot];
		if (isp == NULL) {
			isp = spic->sp_intsrc;
			if (isp == NULL) {
				return ENOMEM;
			}
		} else {
			if (isp->is_pin != pin) {
				if (pic == &i8259_template) {
					return (EINVAL);
				}
				goto other;
			}
		}
duplicate:
		if (pic == &i8259_template) {
			idtvec = ICU_OFFSET + legacy_irq;
		} else {
#ifdef IOAPIC_HWMASK
			if (level > isp->is_maxlevel) {
#else
			if (isp->is_minlevel == 0 || level < isp->is_minlevel) {
#endif
				idtvec = idt_vec_alloc(APIC_LEVEL(level), IDT_INTR_HIGH);
				if (idtvec == 0) {
					return (EBUSY);
				}
			} else {
				//idtvec = isp->is_idtvec;
			}
		}
	} else {
other:
		ci = &cpu_info;
		error = intr_allocate_slot_cpu(spic, ci, pin, &slot);
		if (error == 0) {
			goto found;
		}
		for (CPU_INFO_FOREACH(cii, ci)) {
			if (CPU_IS_PRIMARY(ci)) {
				error = intr_allocate_slot_cpu(spic, ci, pin, &slot);
				if (error == 0) {
					goto found;
				}
			}
		}
		return (EBUSY);
found:
		idtvec = idt_vec_alloc(APIC_LEVEL(level), IDT_INTR_HIGH);
		if (idtvec == 0) {
			intrsrc[slot] = NULL;
			return (EBUSY);
		}
	}

	*idt_slot = idtvec;
	*index = slot;
	*cip = ci;
	return (0);
}
