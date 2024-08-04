/*	$NetBSD: x86_softintr.c,v 1.3 2020/05/08 21:43:54 ad Exp $	*/

/*
 * Copyright (c) 2007, 2008, 2009, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran, and by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
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
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/user.h>

//#include <dev/core/isa/isareg.h>
//#include <dev/core/isa/isavar.h>

#include <machine/cpuinfo.h>
#include <machine/gdt.h>
//#include <machine/intr.h>
#include <machine/pic.h>
#include <machine/apic/i8259.h>

#include "ioapic.h"
#include "lapic.h"
#include "pci.h"

#if NIOAPIC > 0
#include <machine/apic/ioapicvar.h>
#include <machine/mpbiosvar.h>
#endif

#if NLAPIC > 0
#include <machine/apic/lapicvar.h>
#endif

#if NPCI > 0
#include <dev/core/pci/ppbreg.h>
#endif

struct intrsource 	*intrsrc[MAX_INTR_SOURCES];
struct intrhand 	*intrhand[MAX_INTR_SOURCES];
int 				intrtype[MAX_INTR_SOURCES];
int 				intrmask[MAX_INTR_SOURCES];
int 				intrlevel[MAX_INTR_SOURCES];
int					imask[NIPL];
int					iunmask[NIPL];

void				intr_legacy_vectors(void);
void				init_intrmask(void);
char 				*intr_typename(int);

#if NIOAPIC > 0
static int intr_scan_bus(int, int, int *);
#if NPCI > 0
static int intr_find_pcibridge(int, pcitag_t *, pci_chipset_tag_t *);
#endif
#endif

void
intr_default_setup(void)
{
	/* icu & apic vectors */
	intr_legacy_vectors();
}

void
intr_legacy_vectors(void)
{
	int i, idx;
	for(i = 0; i < NUM_LEGACY_IRQS; i++) {
		idx = ICU_OFFSET + i;
		idt_vec_reserve(idx);
		idt_vec_set(idx, legacy_stubs[i].ist_entry);
	}
	i8259_default_setup();
}

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

void
intr_calculatemasks()
{
	int irq, level, unusedirqs;
	struct intrhand **p, *q;

	/* First, figure out which levels each IRQ uses. */
	unusedirqs = 0xffffffff;
	for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
		int levels = 0;
		if(intrsrc[irq] == NULL) {
			intrlevel[irq] = 0;
			continue;
		}

		for (p = &intrhand[irq]; (q = *p) != NULL; p = &q->ih_next) {
			levels |= 1U << q->ih_level;
		}
		intrlevel[irq] = levels;
		if (levels) {
			unusedirqs &= ~(1U << irq);
		}
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		int irqs = 0;
		for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
			if (intrlevel[irq] & (1U << level)) {
				irqs |= 1U << irq;
			}
		}
		imask[level] = irqs | unusedirqs;
	}

	/* Initialize soft interrupt masks */
	init_intrmask();

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	for (level = 0; level < (NIPL-1); level++) {
		imask[level+1] |= imask[level];
	}

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < MAX_INTR_SOURCES; irq++) {
		int irqs = 1 << irq;
		int maxlevel = IPL_NONE;
		int minlevel = IPL_HIGH;

		if(intrsrc[irq] != NULL) {
			continue;
		};

		for (q = intrsrc[irq]->is_handlers; q; q = q->ih_next) {
			irqs |= imask[q->ih_level];
			if (q->ih_level < minlevel) {
				minlevel = q->ih_level;
			}
			if (q->ih_level > maxlevel) {
				maxlevel = q->ih_level;
			}
		}
		intrmask[irq] = irqs;
		intrsrc[irq]->is_maxlevel =  maxlevel;
		intrsrc[irq]->is_minlevel = minlevel;
	}

	/* Lastly, determine which IRQs are actually in use. */
	{
		int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++) {
			if (intrhand[irq]) {
				irqs |= 1 << irq;
			}
		}
		if (irqs >= 0x100) { /* any IRQs >= 8 in use */
			irqs |= 1 << IRQ_SLAVE;
		}
		imen = ~irqs;
		outb(IO_ICU1 + 1, imen);
		outb(IO_ICU2 + 1, imen >> 8);
	}

	for (level = 0; level < NIPL; level++) {
		iunmask[level] = ~imask[level];
	}
}

/*
 * soft interrupt masks
 */

void
init_intrmask(void)
{
#define SOFTINTR_MASK(mask, sir)	((mask) = 1 << (sir))

	/*
	 * Initialize soft interrupt masks to block themselves.
	 */
	SOFTINTR_MASK(imask[IPL_SOFTCLOCK], SIR_CLOCK);
	SOFTINTR_MASK(imask[IPL_SOFTNET], SIR_NET);
	SOFTINTR_MASK(imask[IPL_SOFTSERIAL], SIR_SERIAL);
	SOFTINTR_MASK(imask[IPL_SOFTBIO], SIR_BIO);
	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_NONE];
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_SOFTBIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	imask[IPL_IMP] |= imask[IPL_TTY];

	imask[IPL_AUDIO] |= imask[IPL_IMP];

	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and disk drivers, clock > imp.
	 */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];

	/*
	 * IPL_HIGH must block everything that can manipulate a run queue.
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];

	/*
	 * We need serial drivers to run at the absolute highest priority to
	 * avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];
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
    	ci = cpu_info;
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
    		ci = cpu_info;
#endif
    	}
    	KASSERT(ci != NULL);
    	error = intr_allocate_slot_cpu(spic, ci, pin, &slot, TRUE);
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
fakeintr(spic, fakehand, level)
	struct softpic *spic;
	struct intrhand *fakehand;
	u_int level;
{
	void *si;
	u_int which;

	switch (level) {
	case IPL_SOFTBIO:
		which = I386_SOFTINTR_SOFTBIO;
		break;

	case IPL_SOFTCLOCK:
		which = I386_SOFTINTR_SOFTCLOCK;
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr_distpatch(which);
#endif
		break;

	case IPL_SOFTNET:
		which = I386_SOFTINTR_SOFTNET;
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr_distpatch(which);
#endif
		break;

	case IPL_SOFTSERIAL:
		which = I386_SOFTINTR_SOFTSERIAL;
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr_distpatch(which);
#endif
		break;

	default:
		panic("fakeintr: no interrupt");
	}

	fakehand->ih_pic = softpic_handle_pic(spic);
	fakehand->ih_level = which;
}

void *
intr_establish(irq, type, level, ih_fun, ih_arg, isapic, pictemplate)
	int irq, type, level, pictemplate;
	bool_t isapic;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	register struct softpic *spic;
	register struct intrhand *ih;
	struct cpu_info *ci;
	int error, pin, slot, idtvec;
	void *arg;

	spic = softpic_intr_handler(irq, isapic, pictemplate);
	if(isapic) {
		pin = spic->sp_pins[irq].sp_irq;
		irq = pin;
	}
	error = intr_allocate_slot(spic, &ci, irq, level, &slot, &idtvec, isapic);
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
intr_disestablish(irq, isapic, pictemplate)
	int irq, pictemplate;
	bool_t isapic;
{
	register struct softpic *spic;
	register struct cpu_info *ci;
	int pin, idtvec;

	spic = softpic_intr_handler(irq, isapic, pictemplate);
	ci = cpu_info;
	idtvec = spic->sp_idtvec;
	if(isapic) {
		pin = spic->sp_pins[irq].sp_irq;
		irq = pin;
	}
	softpic_disestablish(spic, ci, irq, idtvec, isapic, pictemplate);
}

#if NIOAPIC > 0
static int intr_scan_bus(int, int, int *);
#if NPCI > 0
static int intr_find_pcibridge(int, pcitag_t *, pci_chipset_tag_t *);
#endif
#endif

/*
 * List to keep track of PCI buses that are probed but not known
 * to the firmware. Used to
 *
 * XXX should maintain one list, not an array and a linked list.
 */
#if (NPCI > 0) && (NIOAPIC > 0)
struct intr_extra_bus {
	int 						bus;
	pcitag_t 					*pci_bridge_tag;
	pci_chipset_tag_t 			pci_chipset_tag;
	LIST_ENTRY(intr_extra_bus) 	list;
};

LIST_HEAD(, intr_extra_bus) intr_extra_buses =
    LIST_HEAD_INITIALIZER(intr_extra_buses);

void
intr_add_pcibus(struct pcibus_attach_args *pba)
{
	struct intr_extra_bus *iebp;

	iebp = malloc(sizeof(struct intr_extra_bus), M_TEMP, M_WAITOK);
	iebp->bus = pba->pba_bus;
	iebp->pci_chipset_tag = pba->pba_pc;
	iebp->pci_bridge_tag = pba->pba_bridgetag;
	LIST_INSERT_HEAD(&intr_extra_buses, iebp, list);
}

static int
intr_find_pcibridge(int bus, pcitag_t *pci_bridge_tag,pci_chipset_tag_t *pci_chipset_tag)
{
	struct intr_extra_bus *iebp;
	struct mp_bus *mpb;

	if (bus < 0)
		return (ENOENT);

	if (bus < mp_nbus) {
		mpb = &mp_busses[bus];
		if (mpb->mb_pci_bridge_tag == NULL)
			return ENOENT;
		*pci_bridge_tag = *mpb->mb_pci_bridge_tag;
		*pci_chipset_tag = mpb->mb_pci_chipset_tag;
		return (0);
	}

	LIST_FOREACH(iebp, &intr_extra_buses, list) {
		if (iebp->bus == bus) {
			if (iebp->pci_bridge_tag == NULL)
				return (ENOENT);
			*pci_bridge_tag = *iebp->pci_bridge_tag;
			*pci_chipset_tag = iebp->pci_chipset_tag;
			return (0);
		}
	}
	return (ENOENT);
}
#endif

#if NIOAPIC > 0
int
intr_find_mpmapping(int bus, int pin, int *handle)
{
#if NPCI > 0
	int dev, func;
	pcitag_t pci_bridge_tag;
	pci_chipset_tag_t pci_chipset_tag;
#endif

#if NPCI > 0
	while (intr_scan_bus(bus, pin, handle) != 0) {
		if (intr_find_pcibridge(bus, &pci_bridge_tag, &pci_chipset_tag) != 0) {
			return (ENOENT);
		}
		dev = pin >> 2;
		pin = pin & 3;
		pin = PPB_INTERRUPT_SWIZZLE(pin + 1, dev) - 1;
		pci_decompose_tag(pci_chipset_tag, pci_bridge_tag, &bus, &dev, &func);
		pin |= (dev << 2);
	}
	return (0);
#else
	return (intr_scan_bus(bus, pin, handle));
#endif
}

static int
intr_scan_bus(int bus, int pin, int *handle)
{
	struct mp_intr_map *mip, *intrs;

	if (bus < 0 || bus >= mp_nbus)
		return ENOENT;

	intrs = mp_busses[bus].mb_intrs;
	if (intrs == NULL)
		return (ENOENT);

	for (mip = intrs; mip != NULL; mip = mip->next) {
		if (mip->bus_pin == pin) {
			*handle = mip->ioapic_ih;
			return (0);
		}
	}
	return (ENOENT);
}
#endif

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
