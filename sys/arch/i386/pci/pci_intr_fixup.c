/*	$NetBSD: pci_intr_fixup.c,v 1.27.2.1 2004/04/28 05:19:04 jmc Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * PCI Interrupt Router support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_intr_fixup.c,v 1.27.2.1 2004/04/28 05:19:04 jmc Exp $");

#include "opt_pcibios.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/null.h>

#include <machine/bios.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcidevs.h>

#include <machine/pci/pci_machdep.h>
#include <i386/pci/pcibios.h>
#include <i386/pci/pci_intr_fixup.h>

struct pciintr_link_map {
	int 							link;
	int 							clink;
	int							irq;
	uint16_t						bitmap;
	int 							fixup_stage;
	SIMPLEQ_ENTRY(pciintr_link_map) list;
};

pciintr_icu_tag_t 		pciintr_icu_tag;
pciintr_icu_handle_t 	pciintr_icu_handle;

#ifdef PCIBIOS_IRQS_HINT
int pcibios_irqs_hint = PCIBIOS_IRQS_HINT;
#endif

struct pciintr_link_map *pciintr_link_lookup(int);
struct pciintr_link_map *pciintr_link_alloc(struct PIR_entry *, int);
struct PIR_entry *pciintr_pir_lookup(int, int);
static int pciintr_bitmap_count_irq(int, int *);
static int pciintr_bitmap_find_lowest_irq(int, int *);
int	pciintr_link_init(void);
#ifdef PCIBIOS_INTR_GUESS
int	pciintr_guess_irq(void);
#endif
int	pciintr_link_fixup(void);
int	pciintr_link_route(u_int16_t *);
int	pciintr_irq_release(u_int16_t *);
int	pciintr_header_fixup(pci_chipset_tag_t);
void pciintr_do_header_fixup(pci_chipset_tag_t, pcitag_t, void*);

SIMPLEQ_HEAD(, pciintr_link_map) pciintr_link_map_list;

const struct pciintr_icu_table {
	pci_vendor_id_t		piit_vendor;
	pci_product_id_t 	piit_product;
	int (*piit_init)(pci_chipset_tag_t, bus_space_tag_t, pcitag_t, pciintr_icu_tag_t *, pciintr_icu_handle_t *);
} pciintr_icu_table[] = {
#ifdef notyet
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82371MX,
	  piix_init },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82371AB_ISA,
	  piix_init },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82371FB_ISA,
	  piix_init },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82371SB_ISA,
	  piix_init },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801AA_LPC,
	  piix_init },			/* ICH */
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801AB_LPC,
	  piix_init },			/* ICH0 */
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801BA_LPC,
	  ich_init },			/* ICH2 */
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801BAM_LPC,
	  ich_init },			/* ICH2M */
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801CA_LPC,
	  ich_init },			/* ICH3S */
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801CAM_LPC,
	  ich_init },			/* ICH3M */
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801DB_LPC,
	  ich_init },			/* ICH4 */
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801DB_ISA,
	  ich_init },			/* ICH4M */
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801EB_LPC,
	  ich_init },			/* ICH5 */

	{ PCI_VENDOR_OPTI,	PCI_PRODUCT_OPTI_82C558,
	  opti82c558_init },
	{ PCI_VENDOR_OPTI,	PCI_PRODUCT_OPTI_82C700,
	  opti82c700_init },

	{ PCI_VENDOR_VIATECH,	PCI_PRODUCT_VIATECH_VT82C586_ISA,
	  via82c586_init },
	{ PCI_VENDOR_VIATECH,   PCI_PRODUCT_VIATECH_VT82C596A,
	  via82c586_init },
	{ PCI_VENDOR_VIATECH,	PCI_PRODUCT_VIATECH_VT82C686A_ISA,
	  via82c586_init },

	{ PCI_VENDOR_SIS,	PCI_PRODUCT_SIS_85C503,
	  sis85c503_init },

	{ PCI_VENDOR_AMD,	PCI_PRODUCT_AMD_PBC756_PMC,
	  amd756_init },

	{ PCI_VENDOR_ALI,	PCI_PRODUCT_ALI_M1543,
	  ali1543_init },
#endif

	{ 0,			0,
	  NULL },
};

const struct pciintr_icu_table *pciintr_icu_lookup(pcireg_t);

const struct pciintr_icu_table *
pciintr_icu_lookup(id)
	pcireg_t id;
{
	const struct pciintr_icu_table *piit;

	for (piit = pciintr_icu_table;
	     piit->piit_init != NULL;
	     piit++) {
		if (PCI_VENDOR(id) == piit->piit_vendor &&
		    PCI_PRODUCT(id) == piit->piit_product)
			return (piit);
	}

	return (NULL);
}

struct pciintr_link_map *
pciintr_link_lookup(link_id)
	int link_id;
{
	struct pciintr_link_map *l;

	SIMPLEQ_FOREACH(l, &pciintr_link_map_list, list) {
		if (l->link == link_id) {
			return (l);
		}
	}
	return (NULL);
}

struct pciintr_link_map *
pciintr_link_alloc(entry, pin)
	struct PIR_entry *entry;
	int pin;
{
	struct pciintr_link_map *l, *lstart;
	struct PIR_intpin *intpin;
	int link, clink, irq;

	intpin = &entry->pe_intpin[pin];
	link = intpin->link;
	if (pciintr_icu_tag != NULL) {
		if (pciintr_icu_getclink(pciintr_icu_tag, pciintr_icu_handle, link, &clink) != 0) {
#ifdef DIAGNOSTIC
			printf("pciintr_link_alloc: bus %d device %d: "
					"link 0x%02x invalid\n", entry->pe_bus,
					PIR_DEVFUNC_DEVICE(entry->pe_device), link);
#endif
			return (NULL);
		}

		if (pciintr_icu_get_intr(pciintr_icu_tag, pciintr_icu_handle, clink, &irq) != 0) {
#ifdef DIAGNOSTIC
			printf("pciintr_link_alloc: "
				"bus %d device %d link 0x%02x: "
				"PIRQ 0x%02x invalid\n", entry->pe_bus,
				PIR_DEVFUNC_DEVICE(entry->pe_device), link, clink);
#endif
			return (NULL);
		}
	}
	l = malloc(sizeof(*l), M_DEVBUF, M_NOWAIT);
	if (l == NULL) {
		panic("pciintr_link_alloc");
	}
	memset(l, 0, sizeof(*l));

	l->link = link;
	l->bitmap = intpin->irqs;
	if (pciintr_icu_tag != NULL) { 	/* compatible PCI ICU found */
		l->clink = clink;
		l->irq = irq; 				/* maybe I386_PCI_INTERRUPT_LINE_NO_CONNECTION */
	} else {
		l->clink = link; 			/* only for PCIBIOSVERBOSE diagnostic */
		l->irq = I386_PCI_INTERRUPT_LINE_NO_CONNECTION;
	}
	lstart = SIMPLEQ_FIRST(&pciintr_link_map_list);
	if (lstart == NULL || lstart->link < l->link) {
		SIMPLEQ_INSERT_TAIL(&pciintr_link_map_list, l, list);
	} else {
		SIMPLEQ_INSERT_HEAD(&pciintr_link_map_list, l, list);
	}
	return (l);
}

struct PIR_entry *
pciintr_pir_lookup(bus, device)
	int bus, device;
{
	struct PIR_entry *entry;
	int i;

	if (pci_route_table == NULL) {
		return (NULL);
	}
	for (i = 0; i < pci_route_count; i++) {
		entry = &pci_route_table->pt_entry[i];
		if(entry->pe_bus == bus && PIR_DEVFUNC_DEVICE(entry->pe_device) == device) {
			return (entry);
		}
	}
	return (NULL);
}

static int
pciintr_bitmap_count_irq(irq_bitmap, irqp)
	int irq_bitmap, *irqp;
{
	int i, bit, count = 0, irq = I386_PCI_INTERRUPT_LINE_NO_CONNECTION;

	if (irq_bitmap != 0) {
		for (i = 0, bit = 1; i < 16; i++, bit <<= 1) {
			if (irq_bitmap & bit) {
				irq = i;
				count++;
			}
		}
	}
	*irqp = irq;
	return (count);
}

static int
pciintr_bitmap_find_lowest_irq(irq_bitmap, irqp)
	int irq_bitmap, *irqp;
{
	int i, bit;

	if (irq_bitmap != 0) {
		for (i = 0, bit = 1; i < 16; i++, bit <<= 1) {
			if (irq_bitmap & bit) {
				*irqp = i;
				return (1); /* found */
			}
		}
	}
	return (0); /* not found */
}

int
pciintr_link_init()
{
	struct PIR_entry 	*entry;
	struct PIR_intpin 	*intpin;
	struct pciintr_link_map *l;
	int i, pin, link;

	if (pci_route_table == NULL) {
		/* No PIR table; can't do anything. */
		printf("pciintr_link_init: no PIR table\n");
		return (1);
	}

	SIMPLEQ_INIT(&pciintr_link_map_list);

	for (i = 0; i < pci_route_count; i++) {
		entry = &pci_route_table->pt_entry[i];
		for (pin = 0; pin < PCI_INTERRUPT_PIN_MAX; pin++) {
			intpin = &entry->pe_intpin[pin];
			link = intpin->link;
			if (link == 0) {
				/* No connection for this pin. */
				continue;
			}
			/*
			 * Multiple devices may be wired to the same
			 * interrupt; check to see if we've seen this
			 * one already.  If not, allocate a new link
			 * map entry and stuff it in the map.
			 */
			l = pciintr_link_lookup(link);
			if (l == NULL) {
				(void) pciintr_link_alloc(entry, pin);
			} else if (entry->pe_intpin[pin].irqs != l->bitmap) {
				/*
				 * violates PCI IRQ Routing Table Specification
				 */
#ifdef DIAGNOSTIC
				printf("pciintr_link_init: "
						"bus %d device %d link 0x%02x: "
						"bad irq bitmap 0x%04x, "
						"should be 0x%04x\n", entry->pe_bus,
						PIR_DEVFUNC_DEVICE(entry->pe_device), link,
						entry->pe_intpin[pin].irqs, l->bitmap);
#endif
				/* safer value. */
				l->bitmap &= entry->pe_intpin[pin].irqs;
				/* XXX - or, should ignore this entry? */
			}
		}
	}
	return (0);
}

#ifdef PCIBIOS_INTR_GUESS
/*
 * No compatible PCI ICU found.
 * Hopes the BIOS already setup the ICU.
 */
int
pciintr_guess_irq()
{
	struct pciintr_link_map *l;
	int irq, guessed = 0;

	/*
	 * Stage 1: If only one IRQ is available for the link, use it.
	 */
	SIMPLEQ_FOREACH(l, &pciintr_link_map_list, list) {
		if (l->irq != I386_PCI_INTERRUPT_LINE_NO_CONNECTION)
			continue;
		if (pciintr_bitmap_count_irq(l->bitmap, &irq) == 1) {
			l->irq = irq;
			l->fixup_stage = 1;
#ifdef PCIINTR_DEBUG
			printf("pciintr_guess_irq (stage 1): "
			    "guessing PIRQ 0x%02x to be IRQ %d\n",
			    l->clink, l->irq);
#endif
			guessed = 1;
		}
	}

	return (guessed ? 0 : -1);
}
#endif /* PCIBIOS_INTR_GUESS */

int
pciintr_link_fixup()
{
	struct pciintr_link_map *l;
	int irq;
	u_int16_t pciirq = 0;

	/*
	 * First stage: Attempt to connect PIRQs which aren't
	 * yet connected.
	 */
	SIMPLEQ_FOREACH(l, &pciintr_link_map_list, list) {
		if (l->irq != I386_PCI_INTERRUPT_LINE_NO_CONNECTION) {
			/*
			 * Interrupt is already connected.  Don't do
			 * anything to it.
			 * In this case, l->fixup_stage == 0.
			 */
			pciirq |= 1 << l->irq;
#ifdef PCIINTR_DEBUG
			printf("pciintr_link_fixup: PIRQ 0x%02x already "
			    "connected to IRQ %d\n", l->clink, l->irq);
#endif
			continue;
		}
		/*
		 * Interrupt isn't connected.  Attempt to assign it to an IRQ.
		 */
#ifdef PCIINTR_DEBUG
		printf("pciintr_link_fixup: PIRQ 0x%02x not connected",
		    l->clink);
#endif
		/*
		 * Just do the easy case now; we'll defer the harder ones
		 * to Stage 2.
		 */
		if (pciintr_bitmap_count_irq(l->bitmap, &irq) == 1) {
			l->irq = irq;
			l->fixup_stage = 1;
			pciirq |= 1 << irq;
#ifdef PCIINTR_DEBUG
			printf(", assigning IRQ %d", l->irq);
#endif
		}
#ifdef PCIINTR_DEBUG
		printf("\n");
#endif
	}

	/*
	 * Stage 2: Attempt to connect PIRQs which we didn't
	 * connect in Stage 1.
	 */
	SIMPLEQ_FOREACH(l, &pciintr_link_map_list, list) {
		if (l->irq != I386_PCI_INTERRUPT_LINE_NO_CONNECTION)
			continue;
		if (pciintr_bitmap_find_lowest_irq(l->bitmap & pciirq, &l->irq)) {
			/*
			 * This IRQ is a valid PCI IRQ already
			 * connected to another PIRQ, and also an
			 * IRQ our PIRQ can use; connect it up!
			 */
			l->fixup_stage = 2;
#ifdef PCIINTR_DEBUG
			printf("pciintr_link_fixup (stage 2): "
			       "assigning IRQ %d to PIRQ 0x%02x\n",
			       l->irq, l->clink);
#endif
		}
	}

#ifdef PCIBIOS_IRQS_HINT
	/*
	 * Stage 3: The worst case. I need configuration hint that
	 * user supplied a mask for the PCI irqs
	 */
	SIMPLEQ_FOREACH(l, &pciintr_link_map_list, list) {
		if (l->irq != I386_PCI_INTERRUPT_LINE_NO_CONNECTION)
			continue;
		if (pciintr_bitmap_find_lowest_irq(
		    l->bitmap & pcibios_irqs_hint, &l->irq)) {
			l->fixup_stage = 3;
#ifdef PCIINTR_DEBUG
			printf("pciintr_link_fixup (stage 3): "
			       "assigning IRQ %d to PIRQ 0x%02x\n",
			       l->irq, l->clink);
#endif
		}
	}
#endif /* PCIBIOS_IRQS_HINT */

	return (0);
}

int
pciintr_link_route(pciirq)
	u_int16_t *pciirq;
{
	struct pciintr_link_map *l;
	int rv = 0;

	*pciirq = 0;

	SIMPLEQ_FOREACH(l, &pciintr_link_map_list, list) {
		if (l->fixup_stage == 0) {
			if (l->irq == I386_PCI_INTERRUPT_LINE_NO_CONNECTION) {
				/* Appropriate interrupt was not found. */
#ifdef DIAGNOSTIC
				printf("pciintr_link_route: "
				    "PIRQ 0x%02x: no IRQ, try "
				    "\"options PCIBIOS_IRQS_HINT=0x%04x\"\n",
				    l->clink,
				    /* suggest irq 9/10/11, if possible */
				    (l->bitmap & 0x0e00) ? (l->bitmap & 0x0e00)
				    : l->bitmap);
#endif
			} else {
				/* BIOS setting has no problem */
#ifdef PCIINTR_DEBUG
				printf("pciintr_link_route: "
				    "route of PIRQ 0x%02x -> "
				    "IRQ %d preserved BIOS setting\n",
				    l->clink, l->irq);
#endif
				*pciirq |= (1 << l->irq);
			}
			continue; /* nothing to do. */
		}

		if (pciintr_icu_set_intr(pciintr_icu_tag, pciintr_icu_handle, l->clink, l->irq) != 0
				|| pciintr_icu_set_trigger(pciintr_icu_tag, pciintr_icu_handle, l->irq, IST_LEVEL) != 0) {
			printf("pciintr_link_route: route of PIRQ 0x%02x -> IRQ %d failed\n", l->clink, l->irq);
			rv = 1;
		} else {
			/*
			 * Succssfully routed interrupt.  Mark this as
			 * a PCI interrupt.
			 */
			*pciirq |= (1 << l->irq);
		}
	}

	return (rv);
}

int
pciintr_irq_release(pciirq)
	u_int16_t *pciirq;
{
	int i, bit;

	for (i = 0, bit = 1; i < 16; i++, bit <<= 1) {
		if ((*pciirq & bit) == 0) {
			(void) pciintr_icu_set_trigger(pciintr_icu_tag, pciintr_icu_handle, i, IST_EDGE);
		}
	}

	return (0);
}

int
pciintr_header_fixup(pc)
	pci_chipset_tag_t pc;
{
	PCIBIOS_PRINTV(("------------------------------------------\n"));
	PCIBIOS_PRINTV(("  device vendor product pin PIRQ IRQ stage\n"));
	PCIBIOS_PRINTV(("------------------------------------------\n"));
	pci_device_foreach(pc, pcibios_max_bus, pciintr_do_header_fixup, NULL);
	PCIBIOS_PRINTV(("------------------------------------------\n"));

	return (0);
}

void
pciintr_do_header_fixup(pc, tag, context)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	void *context;
{
	struct PIR_entry 	*entry;
	struct PIR_intpin 	*intpin;
	struct pciintr_link_map *l;
	int pin, irq, link;
	int bus, device, function;
	pcireg_t intr, id;

	pci_decompose_tag(pc, tag, &bus, &device, &function);
	id = pci_conf_read(pc, tag, PCI_ID_REG);

	intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
	pin = PCI_INTERRUPT_PIN(intr);
	irq = PCI_INTERRUPT_LINE(intr);

#if 0
	if (pin == 0) {
		/*
		 * No interrupt used.
		 */
		return;
	}
#endif

	entry = pciintr_pir_lookup(bus, device);
	intpin = &entry->pe_intpin[pin - 1];
	if (entry == NULL || (link = intpin->link) == 0) {
		/*
		 * Interrupt not connected; no
		 * need to change.
		 */
		return;
	}

	l = pciintr_link_lookup(link);
	if (l == NULL) {
#ifdef PCIINTR_DEBUG
		/*
		 * No link map entry.
		 * Probably pciintr_icu_getclink() or pciintr_icu_get_intr()
		 * was failed.
		 */
		printf("pciintr_header_fixup: no entry for link 0x%02x "
				"(%d:%d:%d:%c)\n", link, bus, device, function, '@' + pin);
#endif
		return;
	}

#ifdef PCIBIOSVERBOSE
	if (pcibiosverbose) {
		printf("%03d:%02d:%d 0x%04x 0x%04x   %c  0x%02x", bus, device, function,
				PCI_VENDOR(id), PCI_PRODUCT(id), '@' + pin, l->clink);
		if (l->irq == I386_PCI_INTERRUPT_LINE_NO_CONNECTION)
			printf("   -");
		else
			printf(" %3d", l->irq);
		printf("  %d   ", l->fixup_stage);
	}
#endif
	/*
	 * IRQs 14 and 15 are reserved for PCI IDE interrupts; don't muck
	 * with them.
	 */
	if (irq == 14 || irq == 15) {
		PCIBIOS_PRINTV((" WARNING: ignored\n"));
		return;
	}

	if (l->irq == I386_PCI_INTERRUPT_LINE_NO_CONNECTION) {
		/* Appropriate interrupt was not found. */
		if (pciintr_icu_tag == NULL &&
		    irq != 0 && irq != I386_PCI_INTERRUPT_LINE_NO_CONNECTION) {
			/*
			 * Do not print warning,
			 * if no compatible PCI ICU found,
			 * but the irq is already assigned by BIOS.
			 */
			PCIBIOS_PRINTV(("\n"));
		} else {
			PCIBIOS_PRINTV((" WARNING: missing IRQ\n"));
		}
		return;
	}

	if (l->irq == irq) {
		/* don't have to reconfigure */
		PCIBIOS_PRINTV((" already assigned\n"));
		return;
	}

	if (irq == 0 || irq == I386_PCI_INTERRUPT_LINE_NO_CONNECTION) {
		PCIBIOS_PRINTV((" fixed up\n"));
	} else {
		/* routed by BIOS, but inconsistent */
#ifdef PCIBIOS_INTR_FIXUP_FORCE
		/* believe PCI IRQ Routing table */
		PCIBIOS_PRINTV((" WARNING: overriding irq %d\n", irq));
#else
		/* believe PCI Interrupt Configuration Register (default) */
		PCIBIOS_PRINTV((" WARNING: preserving irq %d\n", irq));
		return;
#endif
	}

	intr &= ~(PCI_INTERRUPT_LINE_MASK << PCI_INTERRUPT_LINE_SHIFT);
	intr |= (l->irq << PCI_INTERRUPT_LINE_SHIFT);
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, intr);
}

static u_int16_t
pciintr_compat_router(ph, id)
	struct PIR_header *ph;
	pcireg_t id;
{
	u_int16_t compat_router;

	ph->ph_router_vendor = PCI_VENDOR(id);
	ph->ph_router_device = PCI_PRODUCT(id);
	if(ph->ph_router_vendor != 0 && ph->ph_router_device != 0) {
		compat_router = id;
		return (compat_router);
	}
	return (0);
}

int
pci_intr_fixup(pc, iot, pciirq)
	pci_chipset_tag_t pc;
	bus_space_tag_t iot;
	u_int16_t *pciirq;
{
	const struct pciintr_icu_table *piit;
	struct PIR_header pth;
	u_int32_t compat_router;
	pcitag_t icutag;
	pcireg_t icuid;

	/*
	 * Attempt to initialize our PCI interrupt router.  If
	 * the PIR Table is present in ROM, use the location
	 * specified by the PIR Table, and use the compat ID,
	 * if present.  Otherwise, we have to look for the router
	 * ourselves (the PCI-ISA bridge).
	 *
	 * A number of buggy BIOS implementations leave the router
	 * entry as 000:00:0, which is typically not the correct
	 * device/function.  If the router device address is set to
	 * this value, and the compatible router entry is undefined
	 * (zero is the correct value to indicate undefined), then we
	 * work on the basis it is most likely an error, and search
	 * the entire device-space of bus 0 (but obviously starting
	 * with 000:00:0, in case that really is the right one).
	 */
	piit = NULL;
	pth = pci_route_table->pt_header;
	if (pth.ph_signature != 0
			&& (pth.ph_router_bus != 0 ||
			PIR_DEVFUNC_DEVICE(pth.ph_router_dev_fn) != 0 ||
			PIR_DEVFUNC_FUNCTION(pth.ph_router_dev_fn) != 0)) {
		icutag = pci_make_tag(pc, pth.ph_router_bus,
			    PIR_DEVFUNC_DEVICE(pth.ph_router_dev_fn),
			    PIR_DEVFUNC_FUNCTION(pth.ph_router_dev_fn));
		icuid = pci_conf_read(pc, icutag, PCI_ID_REG);
		compat_router = pciintr_compat_router(&pth, icuid);
		if(compat_router != 0 && (piit = pciintr_icu_lookup(icuid)) == NULL) {
			/*
			 * if we fail to look up an ICU at given
			 * PCI address, try compat ID next.
			 */
			icuid = compat_router;
			piit = pciintr_icu_lookup(icuid);
		}
	} else {
		int device, maxdevs;

		/*
		 * Search configuration space for a known interrupt
		 * router.
		 */
		maxdevs = pci_bus_maxdevs(pc, 0);
		for (device = 0; device < maxdevs; device++) {
			const struct pci_quirkdata *qd;
			int function, nfuncs;
			pcireg_t bhlcr;

			icutag = pci_make_tag(pc, 0, device, 0);
			icuid = pci_conf_read(pc, icutag, PCI_ID_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(icuid) == PCI_VENDOR_INVALID) {
				continue;
			}
			/* XXX Not invalid, but we've done this ~forever. */
			if (PCI_VENDOR(icuid) == 0) {
				continue;
			}

			qd = pci_lookup_quirkdata(PCI_VENDOR(icuid), PCI_PRODUCT(icuid));
			bhlcr = pci_conf_read(pc, icutag, PCI_BHLC_REG);
			if (PCI_HDRTYPE_MULTIFN(bhlcr)
					|| (qd != NULL && (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0)) {
				nfuncs = 8;
			} else {
				nfuncs = 1;
			}
			for (function = 0; function < nfuncs; function++) {
				icutag = pci_make_tag(pc, 0, device, function);
				icuid = pci_conf_read(pc, icutag, PCI_ID_REG);

				/* Invalid vendor ID value? */
				if (PCI_VENDOR(icuid) == PCI_VENDOR_INVALID) {
					continue;
				}
				/* Not invalid, but we've done this ~forever */
				if (PCI_VENDOR(icuid) == 0) {
					continue;
				}

				piit = pciintr_icu_lookup(icuid);
				if (piit != NULL) {
					goto found;
				}
			}
		}
		/*
		 * Invalidate the ICU ID.  If we failed to find the
		 * interrupt router (piit == NULL) we don't want to
		 * display a spurious device address below containing
		 * the product information of the last device we
		 * looked at.
		 */
		icuid = 0;
found:;
	}
	if (piit == NULL) {
		printf("pci_intr_fixup: no compatible PCI ICU found");
		if (pth.ph_signature != 0 && icuid != 0) {
			printf(": ICU vendor 0x%04x product 0x%04x", PCI_VENDOR(icuid), PCI_PRODUCT(icuid));
		}
		printf("\n");
#ifdef PCIBIOS_INTR_GUESS
		if (pciintr_link_init()) {
			return (-1);	/* non-fatal */
		}
		if (pciintr_guess_irq()) {
			return (-1);	/* non-fatal */
		}
		if (pciintr_header_fixup(pc)) {
			return (1);		/* fatal */
		}
		return (0);			/* success! */
#else
		return (-1);		/* non-fatal */
#endif
	}

	/*
	 * Initialize the PCI ICU.
	 */
	if ((*piit->piit_init)(pc, iot, icutag, &pciintr_icu_tag, &pciintr_icu_handle) != 0) {
		return (-1);		/* non-fatal */
	}

	/*
	 * Initialize the PCI interrupt link map.
	 */
	if (pciintr_link_init()) {
		return (-1);		/* non-fatal */
	}

	/*
	 * Fix up the link->IRQ mappings.
	 */
	if (pciintr_link_fixup() != 0) {
		return (-1);		/* non-fatal */
	}

	/*
	 * Now actually program the PCI ICU with the new
	 * routing information.
	 */
	if (pciintr_link_route(pciirq) != 0) {
		return (1);			/* fatal */
	}

	/*
	 * Now that we've routed all of the PIRQs, rewrite the PCI
	 * configuration headers to reflect the new mapping.
	 */
	if (pciintr_header_fixup(pc) != 0) {
		return (1);			/* fatal */
	}

	/*
	 * Free any unused PCI IRQs for ISA devices.
	 */
	if (pciintr_irq_release(pciirq) != 0) {
		return (-1);		/* non-fatal */
	}

	/*
	 * All done!
	 */
	return (0);				/* success! */
}
