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
#include <arch/i386/include/gdt.h>
#include <arch/i386/include/intr.h>
#include <arch/i386/include/pic.h>
#include <arch/i386/include/softpic.h>

/*
 * Determine the idtvec from the irq and level.
 * Notes:
 * If isapic = false: level is ignored and can be set to 0.
 */
void
intrvector(int irq, int level, int *idtvec, bool isapic)
{
    int i, min, max;
    min = 0;
    max = MAX_INTR_SOURCES;
    for(i = min; i < max; i++) {
        intrlevel[i] = i;
        if(level == i) {
            level = intrlevel[i];
            *idtvec = idtvector(irq, level, min, max, isapic);
        }
    }
}

int
intr_allocate_slot_cpu(spic, ci, pin, index, isapic)
    struct softpic *spic;
    struct cpu_info *ci;
    int pin, *index;
    bool_t isapic;
{
    struct intrsource *isp;
    int i, slot, start, max;

    if(isapic == FALSE) {
    	slot = pin;
    } else {
        start = 0;
        max = MAX_INTR_SOURCES;
        slot = -1;

        if (CPU_IS_PRIMARY(ci)) {
			start = NUM_LEGACY_IRQS;
		}
        for (i = start; i < max; i++) {
        	if(intrsrc[i] == NULL) {
        		slot = i;
        		break;
        	}
        }
		if (slot == -1) {
			return EBUSY;
		}
    }
    isp = intrsrc[slot];
	spic->sp_intsrc = isp;
    spic->sp_slot = slot;
    *index = slot;
    return (0);
}

int
intr_allocate_slot(spic, cip, pin, level, index, idtslot, isapic)
    struct softpic *spic;
    struct cpu_info **cip;
    int pin, level, *index, *idtslot;
    bool_t isapic;
{
    CPU_INFO_ITERATOR cii;
	struct cpu_info *ci, *lci;
	struct intrsource *isp;
    int error, slot, idtvec;

    for (CPU_INFO_FOREACH(cii, ci)) {
    	for (slot = 0 ; slot < MAX_INTR_SOURCES ; slot++) {
    		if ((isp = intrsrc[slot]) == NULL) {
    			continue;
    		}
    		if(pin != -1 && isp->is_pin == pin) {
    			*idtslot = spic->sp_idtvec;
    			*index = slot;
    			*cip = ci;
    			return 0;
    		}
    	}
    }

    if(isapic == FALSE) {
		/*
		 * Must be directed to BP.
		 */
    	ci = &cpu_info;
    	error = intr_allocate_slot_cpu(spic, ci, pin, slot, FALSE);
    } else {
		/*
		 * Find least loaded AP/BP and try to allocate there.
		 */
    	ci = NULL;
    	for (CPU_INFO_FOREACH(cii, lci)) {
#if 0
    		if (ci == NULL) {
    			ci = lci;
    		}
#else
    		ci = &cpu_info;
#endif
    	}
    	KASSERT(ci != NULL);
    	error = intr_allocate_slot_cpu(ci, pin, &slot, TRUE);
		/*
		 * If that did not work, allocate anywhere.
		 */
		if (error != 0) {
			for (CPU_INFO_FOREACH(cii, ci)) {
				error = intr_allocate_slot_cpu(spic, ci, pin, &slot, TRUE);
				if (error == 0) {
					break;
				}
			}
		}
    }
	if (error != 0) {
		return (error);
	}
	KASSERT(ci != NULL);

	if(isapic == FALSE) {
		intrvector(pin, level, &idtvec, FALSE);
	} else {
		intrvector(pin, level, &idtvec, TRUE);
	}
	if (idtvec < 0) {
		intrsrc[slot] = NULL;
		spic->sp_intsrc = intrsrc[slot];
		return (EBUSY);
	}
	spic->sp_idtvec = idtvec;
	*idtslot = idtvec;
	*index = slot;
	*cip = ci;
    return (0);
}

void
intr_establish_xcall(spic, irq, idtvec)
	struct softpic *spic;
	int irq, idtvec;
{
	 register struct cpu_info *ci;
	 int error;


	 softpic_establish_xcall(spic, pin, type, level, isapic, pictemplate);
}

/* TODO: */
void *
intr_establish(irq, type, level, ih_fun, ih_arg, isapic, pictemplate)
	int irq, type, level, pictemplate;
	int (*ih_fun)(void *);
	void *ih_arg;
	bool_t isapic;
{
	register struct softpic 	*spic;
	register struct cpu_info 	*ci;
	register struct intrhand 	*ih;
	register struct intrsource 	*is;
	int error, slot, idtvec;

	spic = softpic_establish(intrspic, irq, type, level, isapic, pictemplate);
	ih = spic->sp_inthnd;
	is = spic->sp_intsrc;
	if(ih == NULL) {
		ih = intrhand[irq];
	}
	if(is == NULL) {
		is = intrsrc[irq];
	}
	error = intr_allocate_slot(spic, &ci, irq, level &slot, &idtvec, isapic);
	if (error != 0) {
		free(ih, M_DEVBUF);
		printf("failed to allocate interrupt slot for PIC pin %d\n", irq);
		return (NULL);
	}

	return (ih);
}

/* TODO: */
void
intr_disestablish_xcall()
{

}

/* TODO: */
void
intr_disestablish()
{
	register struct softpic *spic;

	spic = softpic_disestablish(intrspic);
}
