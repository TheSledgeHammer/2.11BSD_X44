/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1997, Stefan Esser <se@freebsd.org>
 * Copyright (c) 2000, Michael Smith <msmith@freebsd.org>
 * Copyright (c) 2000, BSDi
 * All rights reserved.
 * Copyright (c) 2004, John Baldwin <jhb@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/malloc.h>

#include <vm/include/vm.h>
#include <vm/include/pmap.h>

#include <arch/i386/include/bios.h>

#include <devel/arch/i386/include/pci_cfgreg.h>
#include <devel/arch/i386/include/pcibios.h>

#define	NUM_ISA_INTERRUPTS		16

/*
 * A link device.  Loosely based on the ACPI PCI link device.  This doesn't
 * try to support priorities for different ISA interrupts.
 */
struct pci_link {
	TAILQ_ENTRY(pci_link) 	pl_links;
	uint8_t					pl_id;
	uint8_t					pl_irq;
	uint16_t				pl_irqmask;
	int						pl_references;
	int						pl_routed;
};

struct pci_link_lookup {
	struct pci_link			**pci_link_ptr;
	int						bus;
	int						device;
	int						pin;
};

struct pci_dev_lookup {
	uint8_t					link;
	int						bus;
	int						device;
	int						pin;
};

typedef void pir_entry_handler(struct PIR_entry *, struct PIR_intpin *, void *);

static void		pci_print_irqmask(u_int16_t);
static int		pci_pir_biosroute(int, int, int, int, int);
static int		pci_pir_choose_irq(struct pci_link *, int);
static void		pci_pir_create_links(struct PIR_entry *, struct PIR_intpin *, void *);
static void		pci_pir_dump_links(void);
static struct pci_link *pci_pir_find_link(uint8_t);
static void		pci_pir_find_link_handler(struct PIR_entry *, struct PIR_intpin *, void *);
static void		pci_pir_initial_irqs(struct PIR_entry *, struct PIR_intpin *, void *);
static void		pci_pir_parse(void);
static uint8_t	pci_pir_search_irq(int, int, int);
static int		pci_pir_valid_irq(struct pci_link *, int);
static void		pci_pir_walk_table(pir_entry_handler *, void *);

static struct PIR_table *pci_route_table;
static struct device pir_device;
static int pci_route_count, pir_bios_irqs, pir_parsed;
static TAILQ_HEAD(, pci_link) pci_links;
static int pir_interrupt_weight[NUM_ISA_INTERRUPTS];

/* XXX this likely should live in a header file */
/* IRQs 3, 4, 5, 6, 7, 9, 10, 11, 12, 14, 15 */
#define PCI_IRQ_OVERRIDE_MASK 0xdef8

static uint32_t pci_irq_override_mask = PCI_IRQ_OVERRIDE_MASK;

/*
 * Look for the interrupt routing table.
 *
 * We use PCI BIOS's PIR table if it's available. $PIR is the standard way
 * to do this.  Sadly, some machines are not standards conforming and have
 * _PIR instead.  We shrug and cope by looking for both.
 */
void
pcibios_pir_init()
{
	struct PIR_table *pt;
	uint32_t sigaddr;
	int i;
	uint8_t ck, *cv;

	/* Don't try if we've already found a table. */
	if (pci_route_table != NULL) {
		return;
	}

	/* Look for $PIR and then _PIR. */
	sigaddr = bios_sigsearch(0, "$PIR", 4, 16, 0);
	if (sigaddr == 0) {
		sigaddr = bios_sigsearch(0, "_PIR", 4, 16, 0);
	}
	if (sigaddr == 0) {
		return;
	}

	/* If we found something, check the checksum and length. */
	/* XXX - Use pmap_mapdev()? */
	pt = (struct PIR_table *)(uintptr_t)BIOS_PADDRTOVADDR(sigaddr);
	if (pt->pt_header.ph_length <= sizeof(struct PIR_header)) {
		return;
	}
	for (cv = (u_int8_t*) pt, ck = 0, i = 0; i < (pt->pt_header.ph_length); i++) {
		ck += cv[i];
	}
	if (ck != 0) {
		return;
	}
	/* Ok, we've got a valid table. */
	pci_route_table = pt;
	pci_route_count = (pt->pt_header.ph_length - sizeof(struct PIR_header))/sizeof(struct PIR_entry);
}

/*
 * Find the pci_link structure for a given link ID.
 */
static struct pci_link *
pci_pir_find_link(link_id)
	uint8_t link_id;
{
	struct pci_link *pci_link;

	TAILQ_FOREACH(pci_link, &pci_links, pl_links) {
		if (pci_link->pl_id == link_id) {
			return (pci_link);
		}
	}
	return (NULL);
}

/*
 * Find the link device associated with a PCI device in the table.
 */
static void
pci_pir_find_link_handler(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg)
{
	struct pci_link_lookup *lookup;

	lookup = (struct pci_link_lookup*) arg;
	if (entry->pe_bus == lookup->bus && entry->pe_device == lookup->device && intpin - entry->pe_intpin == lookup->pin) {
		*lookup->pci_link_ptr = pci_pir_find_link(intpin->link);
	}
}

/*
 * Check to see if a possible IRQ setting is valid.
 */
static int
pci_pir_valid_irq(struct pci_link *pci_link, int irq)
{
	if (!PCI_INTERRUPT_VALID(irq)) {
		return (0);
	}
	return (pci_link->pl_irqmask & (1 << irq));
}

/*
 * Walk the $PIR executing the worker function for each valid intpin entry
 * in the table.  The handler is passed a pointer to both the entry and
 * the intpin in the entry.
 */
static void
pci_pir_walk_table(pir_entry_handler *handler, void *arg)
{
	struct PIR_entry 	*entry;
	struct PIR_intpin 	*intpin;
	int i, pin;

	entry = &pci_route_table->pt_entry[0];
	for (i = 0; i < pci_route_count; i++, entry++) {
		intpin = &entry->pe_intpin[0];
		for (pin = 0; pin < 4; pin++, intpin++) {
			if (intpin->link != 0) {
				handler(entry, intpin, arg);
			}
		}
	}
}

static void
pci_pir_create_links(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg)
{
	struct pci_link *pci_link;

	pci_link = pci_pir_find_link(intpin->link);
	if (pci_link != NULL) {
		pci_link->pl_references++;
		if (intpin->irqs != pci_link->pl_irqmask) {
			if (bootverbose)
				printf(
						"$PIR: Entry %d.%d.INT%c has different mask for link %#x, merging\n",
						entry->pe_bus, entry->pe_device,
						(intpin - entry->pe_intpin) + 'A', pci_link->pl_id);
			pci_link->pl_irqmask &= intpin->irqs;
		}
	} else {
		pci_link = malloc(sizeof(struct pci_link), M_DEVBUF, M_WAITOK);
		pci_link->pl_id = intpin->link;
		pci_link->pl_irqmask = intpin->irqs;
		pci_link->pl_irq = PCI_INVALID_IRQ;
		pci_link->pl_references = 1;
		pci_link->pl_routed = 0;
		TAILQ_INSERT_TAIL(&pci_links, pci_link, pl_links);
	}
}

/*
 * Look to see if any of the functions on the PCI device at bus/device
 * have an interrupt routed to intpin 'pin' by the BIOS.
 */
static uint8_t
pci_pir_search_irq(pci_chipset_tag_t pc, int bus)
{
	const struct pci_quirkdata *qd;
	pcitag_t tag;
	pcireg_t id, bhlcr;
	int function, nfuncs;
	int device, maxdevs;

	maxdevs = pci_bus_maxdevs(pc, bus);
	for(device = 0; device < maxdevs; device++) {
		tag = pci_make_tag(pc, bus, device, 0);
		id = pci_conf_read(pc, tag, PCI_ID_REG);

		/* Invalid vendor ID value? */
		if (PCI_VENDOR(id) == PCI_VENDOR_INVALID) {
			continue;
		}
		/* XXX Not invalid, but we've done this ~forever. */
		if (PCI_VENDOR(id) == 0) {
			continue;
		}

		qd = pci_lookup_quirkdata(PCI_VENDOR(id), PCI_PRODUCT(id));
		bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
		if (PCI_HDRTYPE_MULTIFN(bhlcr) || (qd != NULL && (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0)) {
			nfuncs = 8;
		} else {
			nfuncs = 1;
		}
		for (function = 0; function < nfuncs; function++) {
			tag = pci_make_tag(pc, bus, device, function);
			id = pci_conf_read(pc, tag, PCI_ID_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(id) == PCI_VENDOR_INVALID) {
				continue;
			}
			/* Not invalid, but we've done this ~forever */
			if (PCI_VENDOR(id) == 0) {
				continue;
			}

			value = id;
			goto error;
		}
	}

error:
	if(value != PCI_INVALID_IRQ) {
		return (value);
	}
	return (id);
}

void
pci_pir_initial(pc, tag, entry, intpin, context)
	pci_chipset_tag_t 	pc;
	pcitag_t 			tag;
	struct PIR_entry 	*entry;
	struct PIR_intpin 	*intpin;
	void 				*context;
{
	struct pci_link *pci_link;
	int pin, irq, link;
	int bus, device, function;
	pcireg_t intr, id;

	pci_decompose_tag(pc, tag, &bus, &device, &function);
	id = pci_conf_read(pc, tag, PCI_ID_REG);

	intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
	pin = PCI_INTERRUPT_PIN(intr);
	irq = PCI_INTERRUPT_LINE(intr);

	pci_link = pci_pir_find_link(intpin->link);

	if (irq == PCI_INVALID_IRQ || irq == pci_link->pl_irq) {
		return;
	}
	if (irq >= NUM_ISA_INTERRUPTS) {
		printf(
				"$PIR: Ignoring invalid BIOS IRQ %d from %d.%d.INT%c for link %#x\n",
				irq, entry->pe_bus, entry->pe_device, pin + 'A',
				pci_link->pl_id);
		return;
	}

	/*
	 * If we don't have an IRQ for this link yet, then we trust the
	 * BIOS, even if it seems invalid from the $PIR entries.
	 */
	if (pci_link->pl_irq == PCI_INVALID_IRQ) {
		if (!pci_pir_valid_irq(pci_link, irq)) {
			printf(
					"$PIR: Using invalid BIOS IRQ %d from %d.%d.INT%c for link %#x\n",
					irq, entry->pe_bus, entry->pe_device, pin + 'A',
					pci_link->pl_id);
			pci_link->pl_irq = irq;
			pci_link->pl_routed = 1;
			return;
		}
	}

	/*
	 * We have an IRQ and it doesn't match the current IRQ for this
	 * link.  If the new IRQ is invalid, then warn about it and ignore
	 * it.  If the old IRQ is invalid and the new IRQ is valid, then
	 * prefer the new IRQ instead.  If both IRQs are valid, then just
	 * use the first one.  Note that if we ever get into this situation
	 * we are having to guess which setting the BIOS actually routed.
	 * Perhaps we should just give up instead.
	 */
	if (!pci_pir_valid_irq(pci_link, irq)) {
		printf("$PIR: BIOS IRQ %d for %d.%d.INT%c is not valid for link %#x\n",
				irq, entry->pe_bus, entry->pe_device, pin + 'A',
				pci_link->pl_id);
	} else if (!pci_pir_valid_irq(pci_link, pci_link->pl_irq)) {
		printf(
				"$PIR: Preferring valid BIOS IRQ %d from %d.%d.INT%c for link %#x to IRQ %d\n",
				irq, entry->pe_bus, entry->pe_device, pin + 'A', pci_link->pl_id, pci_link->pl_irq);
		pci_link->pl_irq = irq;
		pci_link->pl_routed = 1;
	}  else {
		printf(
				"$PIR: BIOS IRQ %d for %d.%d.INT%c does not match link %#x irq %d\n",
				irq, entry->pe_bus, entry->pe_device, pin + 'A',
				pci_link->pl_id, pci_link->pl_irq);
	}

	intr &= ~(PCI_INTERRUPT_LINE_MASK << PCI_INTERRUPT_LINE_SHIFT);
	intr |= (pci_link->pl_irq << PCI_INTERRUPT_LINE_SHIFT);
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, intr);
}

/*
 * Try to initialize IRQ based on this device's IRQ.
 */
static void
pci_pir_initial_irqs(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg)
{
	struct pci_link *pci_link;
	int pin, irq, link;
	int bus, device, function;
	pcireg_t intr, id;

	pin = intpin - entry->pe_intpin;
	pci_link = pci_pir_find_link(intpin->link);
	irq = pci_pir_search_irq(entry->pe_bus, entry->pe_device, pin);

	if (irq == PCI_INVALID_IRQ || irq == pci_link->pl_irq) {
		return;
	}
	if (irq >= NUM_ISA_INTERRUPTS) {
		printf(
				"$PIR: Ignoring invalid BIOS IRQ %d from %d.%d.INT%c for link %#x\n",
				irq, entry->pe_bus, entry->pe_device, pin + 'A', pci_link->pl_id);
		return;
	}

	/*
	 * If we don't have an IRQ for this link yet, then we trust the
	 * BIOS, even if it seems invalid from the $PIR entries.
	 */
	if (pci_link->pl_irq == PCI_INVALID_IRQ) {
		if (!pci_pir_valid_irq(pci_link, irq))
			printf(
					"$PIR: Using invalid BIOS IRQ %d from %d.%d.INT%c for link %#x\n",
					irq, entry->pe_bus, entry->pe_device, pin + 'A', pci_link->pl_id);
		pci_link->pl_irq = irq;
		pci_link->pl_routed = 1;
		return;
	}

	/*
	 * We have an IRQ and it doesn't match the current IRQ for this
	 * link.  If the new IRQ is invalid, then warn about it and ignore
	 * it.  If the old IRQ is invalid and the new IRQ is valid, then
	 * prefer the new IRQ instead.  If both IRQs are valid, then just
	 * use the first one.  Note that if we ever get into this situation
	 * we are having to guess which setting the BIOS actually routed.
	 * Perhaps we should just give up instead.
	 */
	if (!pci_pir_valid_irq(pci_link, irq)) {
		printf("$PIR: BIOS IRQ %d for %d.%d.INT%c is not valid for link %#x\n",
				irq, entry->pe_bus, entry->pe_device, pin + 'A', pci_link->pl_id);
	} else if (!pci_pir_valid_irq(pci_link, pci_link->pl_irq)) {
		printf(
				"$PIR: Preferring valid BIOS IRQ %d from %d.%d.INT%c for link %#x to IRQ %d\n",
				irq, entry->pe_bus, entry->pe_device, pin + 'A', pci_link->pl_id, pci_link->pl_irq);
		pci_link->pl_irq = irq;
		pci_link->pl_routed = 1;
	} else
		printf(
				"$PIR: BIOS IRQ %d for %d.%d.INT%c does not match link %#x irq %d\n",
				irq, entry->pe_bus, entry->pe_device, pin + 'A',
				pci_link->pl_id, pci_link->pl_irq);
}

/*
 * Parse $PIR to enumerate link devices and attempt to determine their
 * initial state.  This could perhaps be cleaner if we had drivers for the
 * various interrupt routers as they could read the initial IRQ for each
 * link.
 */
static void
pci_pir_parse(void)
{
	char tunable_buffer[64];
	struct pci_link *pci_link;
	int i, irq;

	/* Only parse once. */
	if (pir_parsed)
		return;
	pir_parsed = 1;

	/* Enumerate link devices. */
	TAILQ_INIT(&pci_links);
	pci_pir_walk_table(pci_pir_create_links, NULL);
	if (bootverbose) {
		printf("$PIR: Links after initial probe:\n");
		pci_pir_dump_links();
	}

	/*
	 * Check to see if the BIOS has already routed any of the links by
	 * checking each device connected to each link to see if it has a
	 * valid IRQ.
	 */
	pci_pir_walk_table(pci_pir_initial_irqs, NULL);
	if (bootverbose) {
		printf("$PIR: Links after initial IRQ discovery:\n");
		pci_pir_dump_links();
	}
	/*
	 * Allow the user to override the IRQ for a given link device.  We
	 * allow invalid IRQs to be specified but warn about them.  An IRQ
	 * of 255 or 0 clears any preset IRQ.
	 */
	i = 0;
	TAILQ_FOREACH(pci_link, &pci_links, pl_links) {
		snprintf(tunable_buffer, sizeof(tunable_buffer), "hw.pci.link.%#x.irq",
				pci_link->pl_id);
		if (getenv_int(tunable_buffer, &irq) == 0)
			continue;
		if (irq == 0)
			irq = PCI_INVALID_IRQ;
		if (irq != PCI_INVALID_IRQ && !pci_pir_valid_irq(pci_link, irq)
				&& bootverbose)
			printf(
					"$PIR: Warning, IRQ %d for link %#x is not listed as valid\n",
					irq, pci_link->pl_id);
		pci_link->pl_routed = 0;
		pci_link->pl_irq = irq;
		i = 1;
	}
	if (bootverbose && i) {
		printf("$PIR: Links after tunable overrides:\n");
		pci_pir_dump_links();
	}

	/*
	 * Build initial interrupt weights as well as bitmap of "known-good"
	 * IRQs that the BIOS has already used for PCI link devices.
	 */
	TAILQ_FOREACH(pci_link, &pci_links, pl_links) {
		if (!PCI_INTERRUPT_VALID(pci_link->pl_irq))
			continue;
		pir_bios_irqs |= 1 << pci_link->pl_irq;
		pir_interrupt_weight[pci_link->pl_irq] += pci_link->pl_references;
	}
	if (bootverbose) {
		printf("$PIR: IRQs used by BIOS: ");
		pci_print_irqmask(pir_bios_irqs);
		printf("\n");
		printf("$PIR: Interrupt Weights:\n[ ");
		for (i = 0; i < NUM_ISA_INTERRUPTS; i++)
			printf(" %3d", i);
		printf(" ]\n[ ");
		for (i = 0; i < NUM_ISA_INTERRUPTS; i++)
			printf(" %3d", pir_interrupt_weight[i]);
		printf(" ]\n");
	}
}

/*
 * Use the PCI BIOS to route an interrupt for a given device.
 *
 * Input:
 * AX = PCIBIOS_ROUTE_INTERRUPT
 * BH = bus
 * BL = device [7:3] / function [2:0]
 * CH = IRQ
 * CL = Interrupt Pin (0x0A = A, ... 0x0D = D)
 */
static int
pci_pir_biosroute(bus, device, func, pin, irq)
	int bus, device, func, pin, irq;
{
	struct bios_regs args;

	args.eax = PCIBIOS_ROUTE_INTERRUPT;
	args.ebx = (bus << 8) | (device << 3) | func;
	args.ecx = (irq << 8) | (0xa + pin);

	return (bios32(&args, PCIbios.ventry, GSEL(GCODE_SEL, SEL_KPL)));
}

/*
 * Route a PCI interrupt using a link device from the $PIR.
 */
int
pci_pir_route_interrupt(bus, device, func, pin)
	int bus, device, func, pin;
{
	struct pci_link_lookup lookup;
	struct pci_link *pci_link;
	int error, irq;

	if (pci_route_table == NULL)
		return (PCI_INVALID_IRQ);

	/* Lookup link device for this PCI device/pin. */
	pci_link = NULL;
	lookup.bus = bus;
	lookup.device = device;
	lookup.pin = pin - 1;
	lookup.pci_link_ptr = &pci_link;
	pci_pir_walk_table(pci_pir_find_link_handler, &lookup);
	if (pci_link == NULL) {
		printf("$PIR: No matching entry for %d.%d.INT%c\n", bus, device, pin - 1 + 'A');
		return (PCI_INVALID_IRQ);
	}

	/*
	 * Pick a new interrupt if we don't have one already.  We look
	 * for an interrupt from several different sets.  First, if
	 * this link only has one valid IRQ, use that.  Second, we
	 * check the set of PCI only interrupts from the $PIR.  Third,
	 * we check the set of known-good interrupts that the BIOS has
	 * already used.  Lastly, we check the "all possible valid
	 * IRQs" set.
	 */
	if (!PCI_INTERRUPT_VALID(pci_link->pl_irq)) {
		if (pci_link->pl_irqmask != 0 && powerof2(pci_link->pl_irqmask))
			irq = ffs(pci_link->pl_irqmask) - 1;
		else
			irq = pci_pir_choose_irq(pci_link, pci_route_table->pt_header.ph_pci_irqs);
		if (!PCI_INTERRUPT_VALID(irq))
			irq = pci_pir_choose_irq(pci_link, pir_bios_irqs);
		if (!PCI_INTERRUPT_VALID(irq))
			irq = pci_pir_choose_irq(pci_link, pci_irq_override_mask);
		if (!PCI_INTERRUPT_VALID(irq)) {
			if (bootverbose)
				printf("$PIR: Failed to route interrupt for %d:%d INT%c\n", bus,
						device, pin - 1 + 'A');
			return (PCI_INVALID_IRQ);
		}
		pci_link->pl_irq = irq;
	}

	/* Ask the BIOS to route this IRQ if we haven't done so already. */
	if (!pci_link->pl_routed) {
		error = pci_pir_biosroute(bus, device, func, pin - 1, pci_link->pl_irq);

		/* Ignore errors when routing a unique interrupt. */
		if (error && !powerof2(pci_link->pl_irqmask)) {
			printf("$PIR: ROUTE_INTERRUPT failed.\n");
			return (PCI_INVALID_IRQ);
		}
		pci_link->pl_routed = 1;

		/* Ensure the interrupt is set to level/low trigger. */
		KASSERT(pir_device != NULL, ("missing pir device"));
		BUS_CONFIG_INTR(pir_device, pci_link->pl_irq, INTR_TRIGGER_LEVEL, INTR_POLARITY_LOW);
	}
	if (bootverbose)
		printf("$PIR: %d:%d INT%c routed to irq %d\n", bus, device, pin - 1 + 'A', pci_link->pl_irq);
	return (pci_link->pl_irq);
}

/*
 * Try to pick an interrupt for the specified link from the interrupts
 * set in the mask.
 */
static int
pci_pir_choose_irq(struct pci_link *pci_link, int irqmask)
{
	int i, irq, realmask;

	/* XXX: Need to have a #define of known bad IRQs to also mask out? */
	realmask = pci_link->pl_irqmask & irqmask;
	if (realmask == 0)
		return (PCI_INVALID_IRQ);

	/* Find IRQ with lowest weight. */
	irq = PCI_INVALID_IRQ;
	for (i = 0; i < NUM_ISA_INTERRUPTS; i++) {
		if (!(realmask & 1 << i))
			continue;
		if (irq == PCI_INVALID_IRQ ||
		    pir_interrupt_weight[i] < pir_interrupt_weight[irq])
			irq = i;
	}
	if (bootverbose && PCI_INTERRUPT_VALID(irq)) {
		printf("$PIR: Found IRQ %d for link %#x from ", irq, pci_link->pl_id);
		pci_print_irqmask(realmask);
		printf("\n");
	}
	return (irq);
}

static void
pci_print_irqmask(u_int16_t irqs)
{
	int i, first;

	if (irqs == 0) {
		printf("none");
		return;
	}
	first = 1;
	for (i = 0; i < 16; i++, irqs >>= 1)
		if (irqs & 1) {
			if (!first)
				printf(" ");
			else
				first = 0;
			printf("%d", i);
		}
}

/*
 * Display link devices.
 */
static void
pci_pir_dump_links(void)
{
	struct pci_link *pci_link;

	printf("Link  IRQ  Rtd  Ref  IRQs\n");
	TAILQ_FOREACH(pci_link, &pci_links, pl_links) {
		printf("%#4x  %3d   %c   %3d  ", pci_link->pl_id, pci_link->pl_irq,
				pci_link->pl_routed ? 'Y' : 'N', pci_link->pl_references);
		pci_print_irqmask(pci_link->pl_irqmask);
		printf("\n");
	}
}

/*
 * See if any interrupts for a given PCI bus are routed in the PIR.  Don't
 * even bother looking if the BIOS doesn't support routing anyways.  If we
 * are probing a PCI-PCI bridge, then require_parse will be true and we should
 * only succeed if a host-PCI bridge has already attached and parsed the PIR.
 */
int
pci_pir_probe(int bus, int require_parse)
{
	int i;

	if (pci_route_table == NULL || (require_parse && !pir_parsed))
		return (0);
	for (i = 0; i < pci_route_count; i++)
		if (pci_route_table->pt_entry[i].pe_bus == bus)
			return (1);
	return (0);
}
