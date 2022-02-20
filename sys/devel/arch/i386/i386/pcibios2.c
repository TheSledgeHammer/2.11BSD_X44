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

#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <dev/core/pci/pcidevs.h>
#include <dev/core/pci/pcireg.h>
#include <include/pci/pci_machdep.h>

#include <devel/arch/i386/include/pci_cfgreg.h>
#include <devel/arch/i386/include/pcibios.h>

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

struct pci_cfg {
	pci_chipset_tag_t 	pc;
	pcitag_t 			tag;
	pcireg_t			id;
	int					bus;
	int 				minbus;
	int					maxbus;
	int					device;
	int					maxdevs;
	int					function;
	int					nfuncs;
};

struct pci_cfg pci_cfg;

void
pci_cfg_init(elem)
	struct pci_cfg 		*elem;
{
	elem = (struct pci_cfg *)malloc(sizeof(struct pci_cfg *), M_DEVBUF, M_NOWAIT);
}

void
pci_cfg_foreach(elem, pc, maxbus)
	struct pci_cfg 		*elem;
	pci_chipset_tag_t 	pc;
	uint8_t 			maxbus;
{
	pci_cfg_foreach_min(elem, pc, 0, maxbus);
}

void
pci_cfg_foreach_min(elem, pc, minbus, maxbus)
	struct pci_cfg 		*elem;
	pci_chipset_tag_t 	pc;
	uint8_t 			minbus, maxbus;
{
	pcireg_t bhlcr;
	int bus, device, function, nfuncs;

	elem->pc = pc;
	elem->minbus = minbus;
	elem->maxbus = maxbus;

	for (bus = minbus; bus <= maxbus; bus++) {
		elem->bus = bus;
		elem->maxdevs = pci_bus_maxdevs(elem->pc, elem->bus);
		for (elem->device = 0; elem->device < elem->maxdevs; device++) {
			elem->device = device;
			elem->tag = pci_make_tag(elem->pc, elem->bus, elem->device, 0);
			elem->id = pci_conf_read(elem->pc, elem->tag, PCI_ID_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(elem->id) == PCI_VENDOR_INVALID) {
				continue;
			}
			/* XXX Not invalid, but we've done this ~forever. */
			if (PCI_VENDOR(elem->id) == 0) {
				continue;
			}
			bhlcr = pci_conf_read(elem->pc, elem->tag, PCI_BHLC_REG);
			if (PCI_HDRTYPE_MULTIFN(bhlcr)) {
				nfuncs = 8;
			} else {
				nfuncs = 1;
			}
			elem->nfuncs = nfuncs;
			for (function = 0; function < nfuncs; function++) {
				elem->function = function;
				elem->tag = pci_make_tag(elem->pc, elem->bus, elem->device, elem->function);
				elem->id = pci_conf_read(elem->pc, elem->tag, PCI_ID_REG);

				/* Invalid vendor ID value? */
				if (PCI_VENDOR(elem->id) == PCI_VENDOR_INVALID) {
					continue;
				}
				/*
				 * XXX Not invalid, but we've done this
				 * ~forever.
				 */
				if (PCI_VENDOR(elem->id) == 0) {
					continue;
				}
			}
		}
	}
}

int
pci_cfg_lookup_bus(elem, bus)
	struct pci_cfg 	*elem;
	int 					bus;
{
	for (bus = elem->minbus; bus <= elem->maxbus; bus++) {
		if(elem->bus == bus) {
			return (elem->bus);
		}
	}
	return (-1);
}

int
pci_cfg_lookup_device(elem, device)
	struct pci_cfg 	*elem;
	int	device;
{
	for (device = 0; device < elem->maxdevs; device++) {
		if (elem->device == device) {
			return (elem->device);
		}
	}
	return (-1);
}

int
pci_cfg_lookup_function(elem, function)
	struct pci_cfg 	*elem;
	int function;
{
	for(function = 0; function < elem->nfuncs; function++) {
		if (elem->function == function) {
			return (elem->function);
		}
	}
	return (-1);
}

#define PCIADDR_MEM_START		0x0
#define PCIADDR_MEM_END			0xffffffff
#define PCIADDR_PORT_START		0x0
#define PCIADDR_PORT_END		0xffff

/* for ISA devices */
#define PCIADDR_ISAPORT_RESERVE	0x5800 /* empirical value */
#define PCIADDR_ISAMEM_RESERVE	(16 * 1024 * 1024)

struct pciaddr {
	struct extent 	*extent_mem;
	struct extent 	*extent_port;
	bus_addr_t 		mem_alloc_start;
	bus_addr_t 		port_alloc_start;
	int 			nbogus;
};

extern struct pciaddr pciaddr;

int
pci_alloc_mem(int type, bus_addr_t *addr, bus_size_t size)
{
	struct pciaddr *pciaddrmap;
	struct extent *ex;
	bus_addr_t start;
	int error;

	ex = (type == PCI_MAPREG_TYPE_MEM ? pciaddrmap->extent_mem : pciaddrmap->extent_port);

	start = (type == PCI_MAPREG_TYPE_MEM ? pciaddrmap->mem_alloc_start : pciaddrmap->port_alloc_start);
	if (start < ex->ex_start || start + size - 1 >= ex->ex_end) {
		printf("No available resources.\n");
		return (1);
	}

	pciaddr.extent_mem = extent_create("PCI I/O memory space",
			PCIADDR_MEM_START, PCIADDR_MEM_END, 0, 0, EX_NOWAIT);
	KASSERT(pciaddr.extent_mem);

	pciaddr.extent_port = extent_create("PCI I/O port space",
	PCIADDR_PORT_START, PCIADDR_PORT_END, 0, 0, EX_NOWAIT);
	KASSERT(pciaddr.extent_port);

	error = extent_alloc_region(ex, *addr, size, EX_NOWAIT| EX_MALLOCOK);
	if (error) {
		printf("No available resources.\n");
		return (1);
	}

	error = extent_alloc_subregion(ex, start, ex->ex_end, size, size, 0,
	EX_FAST | EX_NOWAIT | EX_MALLOCOK, (u_long*) addr);
	if (error) {
		printf("Resource conflict.\n");
		return (1);
	}
	return (0);
}

void
pci_alloc_isa_mem()
{
	extern vm_offset_t avail_end;
	bus_addr_t start;

	start = i386_round_page(avail_end + 1);
	if (start < PCIADDR_ISAMEM_RESERVE)
		start = PCIADDR_ISAMEM_RESERVE;
	pciaddr.mem_alloc_start = (start + 0x100000 + 1) & ~(0x100000 - 1);
	pciaddr.port_alloc_start = PCIADDR_ISAPORT_RESERVE;
	printf(" Physical memory end: 0x%08x\n PCI memory mapped I/O "
			"space start: 0x%08x\n", (unsigned) avail_end, (unsigned) pciaddr.mem_alloc_start);
}
