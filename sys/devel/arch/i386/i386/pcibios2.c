/*	$NetBSD: pcibios.c,v 1.14.2.1 2004/04/28 05:19:13 jmc Exp $	*/

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
 * Interface to the PCI BIOS and PCI Interrupt Routing table.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcibios.c,v 1.14.2.1 2004/04/28 05:19:13 jmc Exp $");

#include <dev/core/pci/pcidevs.h>
#include <dev/core/pci/pcireg.h>
#include <include/pci/pci_machdep.h>

/*
 * FreeBSD uint32_t value = NetBSD pcireg_t
 */
/*
 * intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
 * pin = PCI_INTERRUPT_PIN(intr);
 */

typedef void (*func_t)(pci_chipset_tag_t, pcitag_t, void *);

void
pci_device_foreach(pci_chipset_tag_t pc, int maxbus, func_t func, void *context)
{
	pci_device_foreach_min(pc, 0, maxbus, func, context);
}

void
pci_device_foreach_min(pc, minbus, maxbus, func, context)
	pci_chipset_tag_t pc;
	int minbus, maxbus;
	func_t func;
	void *context;
{
	int bus, device, function, maxdevs, nfuncs;
	pcireg_t id, bhlcr;
	pcitag_t tag;

	for (bus = minbus; bus <= maxbus; bus++) {
		maxdevs = pci_bus_maxdevs(pc, bus);
		for (device = 0; device < maxdevs; device++) {
			tag = pci_make_tag(pc, bus, device, 0);
			id = pci_conf_read(pc, tag, PCI_ID_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
				continue;
			/* XXX Not invalid, but we've done this ~forever. */
			if (PCI_VENDOR(id) == 0)
				continue;

			bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
			if (PCI_HDRTYPE_MULTIFN(bhlcr)) {
				nfuncs = 8;
			} else {
				nfuncs = 1;
			}

			for (function = 0; function < nfuncs; function++) {
				tag = pci_make_tag(pc, bus, device, function);
				id = pci_conf_read(pc, tag, PCI_ID_REG);

				/* Invalid vendor ID value? */
				if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
					continue;
				/*
				 * XXX Not invalid, but we've done this
				 * ~forever.
				 */
				if (PCI_VENDOR(id) == 0)
					continue;
				(*func)(pc, tag, context);
			}
		}
	}
}

static uint8_t
pci_pir_search_irq(pci_chipset_tag_t pc, int bus, int device, int pin)
{
	uint8_t function, nfuncs;
	int maxdevs;
	pcireg_t id, bhlcr;
	pcitag_t tag;
	func_t fun;

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

	bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
	if((bhlcr & PCI_HDRTYPE(bhlcr)) > 2) {
		return (PCI_INVALID_IRQ);
	}
	if (PCI_HDRTYPE_MULTIFN(bhlcr)) {
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

		/*
		 * XXX Not invalid, but we've done this
		 * ~forever.
		 */
		if (PCI_VENDOR(id) == 0) {
			continue;
		}
		return (id);
	}
	return (PCI_INVALID_IRQ);
}

static void
pci_pir_initial_irqs(pci_chipset_tag_t pc, struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg)
{
	struct pci_link *pci_link;
	uint8_t irq, pin;
	int bus, device, maxdevs;

	pin = intpin - entry->pe_intpin;
	pci_link = pci_pir_find_link(intpin->link);
	irq = pci_pir_search_irq(pc, entry->pe_bus, entry->pe_device, pin);
}
