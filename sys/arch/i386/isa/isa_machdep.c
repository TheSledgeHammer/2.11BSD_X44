/*	$NetBSD: isa_machdep.c,v 1.23.2.1 1997/11/13 08:11:04 mellon Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*-
 * Copyright (c) 1993, 1994, 1996, 1997
 *	Charles M. Hannum.  All rights reserved.
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
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/cpufunc.h>

#include <machine/intr.h>
#include <machine/pic.h>

#include <machine/isa/isa_machdep.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

#include <vm/include/vm_extern.h>

#include "ioapic.h"

#if NIOAPIC > 0
#include <machine/apic/ioapicvar.h>
#include <machine/mpbiosvar.h>
#endif

/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 */
int
isa_nmi(void)
{
	log(LOG_CRIT, "NMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
	return (0);
}

int
isa_intr_alloc(ic, mask, type, irq)
	isa_chipset_tag_t ic;
	int mask;
	int type;
	int *irq;
{
	int i, tmp, bestirq, count;
	struct intrhand 	**p, *q;
	struct intrsource 	*isp;

	if (type == IST_NONE) {
		panic("intr_alloc: bogus type");
	}

	bestirq = -1;
	count = -1;

	/* some interrupts should never be dynamically allocated */
	mask &= 0xdef8;

	/*
	 * XXX some interrupts will be used later (6 for fdc, 12 for pms).
	 * the right answer is to do "breadth-first" searching of devices.
	 */
	mask &= 0xefbf;

	for (i = 0; i < ICU_LEN; i++) {
		if (LEGAL_IRQ(i) == 0 || (mask & (1 << i)) == 0) {
			continue;
		}
		isp = intrsrc[i];

		switch (intrtype[i]) {
		case IST_NONE:
			isp = NULL;
			/*
			 * if nothing's using the irq, just return it
			 */
			*irq = i;
			return (0);

		case IST_EDGE:
		case IST_LEVEL:
			if (type != intrtype[i])
				continue;
			/*
			 * if the irq is shareable, count the number of other
			 * handlers, and if it's smaller than the last irq like
			 * this, remember it
			 *
			 * XXX We should probably also consider the
			 * interrupt level and stick IPL_TTY with other
			 * IPL_TTY, etc.
			 */
			for (p = &intrhand[i], tmp = 0; (q = *p) != NULL; p = &q->ih_next, tmp++) {
				;
			}
			if ((bestirq == -1) || (count > tmp)) {
				bestirq = i;
				count = tmp;
			}
			break;

		case IST_PULSE:
			/* this just isn't shareable */
			continue;
		}
		isp->is_type = intrtype[i];
	}

	if (bestirq == -1) {
		return (1);
	}

	*irq = bestirq;

	return (0);
}

/*
 * Register an interrupt handler.
 */
void *
isa_intr_establish(ic, irq, type, level, ih_fun, ih_arg)
	isa_chipset_tag_t ic;
	int irq;
	int type;
	int level;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	struct intrhand *ih;

	ih = intr_establish(irq, type, level, ih_fun, ih_arg, FALSE, PIC_I8259);

#if NIOAPIC > 0
	struct pic *pic;
	struct ioapic_softc *ioapic = NULL;
	int mpih;
	int pin;

	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, irq, &mpih) == 0 || intr_find_mpmapping(mp_eisa_bus, irq, &mpih) == 0) {
			if (!APIC_IRQ_ISLEGACY(mpih)) {
				pin = APIC_IRQ_PIN(mpih);
				ioapic = ioapic_find(APIC_IRQ_APIC(mpih));
				if (ioapic == NULL) {
					printf("isa_intr_establish: unknown apic %d\n", APIC_IRQ_APIC(mpih));
					return (NULL);
				}
				pic = ioapic->sc_pic;
			}
		}
		return (intr_establish(pin, type, level, ih_fun, ih_arg, TRUE, PIC_IOAPIC));
	}
#endif
	return (ih);
}

/*
 * Deregister an interrupt handler.
 */
void
isa_intr_disestablish(ic, arg)
	isa_chipset_tag_t ic;
	void *arg;
{
	struct intrhand *ih;

	ih = arg;
#if NIOAPIC > 0
	if (ih->ih_irq & APIC_INT_VIA_APIC) {
		intr_disestablish(ih->ih_irq, TRUE, PIC_IOAPIC);
		return;
	}
#endif

	intr_disestablish(ih->ih_irq, FALSE, PIC_I8259);
}

void
isa_attach_hook(parent, self, iba)
	struct device *parent, *self;
	struct isabus_attach_args *iba;
{
	extern int isa_has_been_seen;

	/*
	 * Notify others that might need to know that the ISA bus
	 * has now been attached.
	 */
	if (isa_has_been_seen) {
		panic("isaattach: ISA bus already seen!");
	}
	isa_has_been_seen = 1;
}


void
isa_detach_hook(ic, self)
	isa_chipset_tag_t ic;
	struct device *self;
{
	extern int isa_has_been_seen;

	isa_has_been_seen = 0;
}

int
isa_mem_alloc(t, size, align, boundary, flags, addrp, bshp)
	bus_space_tag_t t;
	bus_size_t size, align;
	bus_addr_t boundary;
	int flags;
	bus_addr_t *addrp;
	bus_space_handle_t *bshp;
{
	/*
	 * Allocate physical address space in the ISA hole.
	 */
	return (bus_space_alloc(t, ISA_HOLE_START, ISA_HOLE_END - 1, size, align, boundary, flags, addrp, bshp));
}

void
isa_mem_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	bus_space_free(t, bsh, size);
}
