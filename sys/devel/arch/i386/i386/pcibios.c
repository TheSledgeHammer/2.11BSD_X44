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

typedef void pir_entry_handler(struct PIR_entry *entry, struct PIR_intpin* intpin, void *arg);

static void		pci_print_irqmask(u_int16_t irqs);
static int		pci_pir_biosroute(int bus, int device, int func, int pin, int irq);
static int		pci_pir_choose_irq(struct pci_link *pci_link, int irqmask);
static void		pci_pir_create_links(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg);
static void		pci_pir_dump_links(void);
static struct pci_link *pci_pir_find_link(uint8_t link_id);
static void		pci_pir_find_link_handler(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg);
static void		pci_pir_initial_irqs(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg);
static void		pci_pir_parse(void);
static uint8_t	pci_pir_search_irq(int bus, int device, int pin);
static int		pci_pir_valid_irq(struct pci_link *pci_link, int irq);
static void		pci_pir_walk_table(pir_entry_handler *handler, void *arg);

static struct PIR_table *pci_route_table;
static int pci_route_count, pir_bios_irqs, pir_parsed;
static TAILQ_HEAD(, pci_link) pci_links;
static int pir_interrupt_weight[NUM_ISA_INTERRUPTS];

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
	if (sigaddr == 0)
		sigaddr = bios_sigsearch(0, "_PIR", 4, 16, 0);
	if (sigaddr == 0)
		return;

	/* If we found something, check the checksum and length. */
	/* XXX - Use pmap_mapdev()? */
	pt = (struct PIR_table *)(uintptr_t)BIOS_PADDRTOVADDR(sigaddr);
	if (pt->pt_header.ph_length <= sizeof(struct PIR_header)) {
		return;
	}
	for (cv = (u_int8_t*) pt, ck = 0, i = 0; i < (pt->pt_header.ph_length); i++)
		ck += cv[i];
	if (ck != 0)
		return;
	/* Ok, we've got a valid table. */
	pci_route_table = pt;
	pci_route_count = (pt->pt_header.ph_length - sizeof(struct PIR_header))
			/ sizeof(struct PIR_entry);
}

/*
 * Find the pci_link structure for a given link ID.
 */
static struct pci_link *
pci_pir_find_link(uint8_t link_id)
{
	struct pci_link *pci_link;

	TAILQ_FOREACH(pci_link, &pci_links, pl_links) {
		if (pci_link->pl_id == link_id)
			return (pci_link);
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
	if (entry->pe_bus == lookup->bus && entry->pe_device == lookup->device
			&& intpin - entry->pe_intpin == lookup->pin)
		*lookup->pci_link_ptr = pci_pir_find_link(intpin->link);
}

/*
 * Check to see if a possible IRQ setting is valid.
 */
static int
pci_pir_valid_irq(struct pci_link *pci_link, int irq)
{

	if (!PCI_INTERRUPT_VALID(irq))
		return (0);
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
		for (pin = 0; pin < 4; pin++, intpin++)
			if (intpin->link != 0)
				handler(entry, intpin, arg);
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
pci_pir_search_irq(pci_chipset_tag_t pc, int bus, int device, int pin)
{

	return (PCI_INVALID_IRQ);
}

/*
 * Try to initialize IRQ based on this device's IRQ.
 */
static void
pci_pir_initial_irqs(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg)
{
	struct pci_link *pci_link;
	uint8_t irq, pin;

	pin = intpin - entry->pe_intpin;
	pci_link = pci_pir_find_link(intpin->link);
	irq = pci_pir_search_irq(entry->pe_bus, entry->pe_device, pin);
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
pci_pir_biosroute(int bus, int device, int func, int pin, int irq)
{
	struct bios_regs args;

	args.eax = PCIBIOS_ROUTE_INTERRUPT;
	args.ebx = (bus << 8) | (device << 3) | func;
	args.ecx = (irq << 8) | (0xa + pin);

	return (bios32(&args, PCIbios.ventry, GSEL(GCODE_SEL, SEL_KPL)));
}
