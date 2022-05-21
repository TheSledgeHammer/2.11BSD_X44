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

static struct lock_object pcicfg_lock;

void
pcibios_init(void)
{
	uint32_t sigaddr;
	uint16_t v;
	static int opened = 0;

	sigaddr = bios_sigsearch(0, "$PCI", 4, 16, 0);
	if (sigaddr == 0)
		sigaddr = bios_sigsearch(0, "_PCI", 4, 16, 0);
	if (sigaddr == 0)
		return;

	if (cfgmech == CFGMECH_NONE && pcireg_cfgopen() == 0) {
		pci_mode_set(cfgmech ? 1 : 2);
	} else {
		pci_mode_set(cfgmech ? 1 : 2);
	}

	v = pcibios_get_version();
	if (v > 0) {
		PRVERB(("pcibios: BIOS version %x.%02x\n", (v & 0xff00) >> 8, v & 0xff));
	}
	simple_lock_init(&pcicfg_lock, "pcicfg_lock");
	opened = 1;

	/* $PIR requires PCI BIOS 2.10 or greater. */
	if (v >= 0x0210) {
		pcibios_pir_init();
	}
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
