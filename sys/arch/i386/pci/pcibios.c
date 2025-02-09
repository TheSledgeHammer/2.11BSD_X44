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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/null.h>

#include <machine/bus.h>
#include <machine/segments.h>
#include <machine/stdarg.h>
#include <machine/vmparam.h>
#include <machine/bios.h>
#include <machine/gdt.h>

#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcidevs.h>

#include <i386/pci/pcibios.h>
#ifdef PCIBIOS_INTR_FIXUP
#include <i386/pci/pci_intr_fixup.h>
#endif
#ifdef PCIBIOS_BUS_FIXUP
#include <i386/pci/pci_bus_fixup.h>
#endif
#ifdef PCIBIOS_ADDR_FIXUP
#include <i386/pci/pci_addr_fixup.h>
#endif

#ifdef PCIBIOSVERBOSE
int	pcibiosverbose = 1;
#endif

struct PIR_table *pci_route_table;
int pci_route_count;
int pcibios_max_bus;

void pcibios_pir_init(void);

#define PRVERB(a) do {			\
	if (bootverbose)			\
		printf a ;				\
} while (0)

static u_int16_t
pcibios_get_version(void)
{
	struct bios_regs args;

	if (PCIbios.ventry == 0) {
		PRVERB(("pcibios: No call entry point\n"));
		return (0);
	}
	args.eax = PCIBIOS_BIOS_PRESENT;
	if (bios32(&args, PCIbios.ventry, GSEL(GCODE_SEL, SEL_KPL))) {
		PRVERB(("pcibios: BIOS_PRESENT call failed\n"));
		return (0);
	}
	if (args.edx != 0x20494350) {
		PRVERB(("pcibios: BIOS_PRESENT didn't return 'PCI ' in edx\n"));
		return (0);
	}
	return (args.ebx & 0xffff);
}

void
pcibios_init(void)
{
	uint32_t sigaddr;
	uint16_t v;

	sigaddr = bios_sigsearch(0, "$PCI", 4, 16, 0);
	if (sigaddr == 0) {
		sigaddr = bios_sigsearch(0, "_PCI", 4, 16, 0);
	}
	if (sigaddr == 0) {
		return;
	}

	pci_mode_set(pci_mode ? 1 : 2);

	v = pcibios_get_version();
	if (v > 0) {
		PRVERB(("pcibios: BIOS version %x.%02x\n", (v & 0xff00) >> 8, v & 0xff));
	}

	/* $PIR requires PCI BIOS 2.10 or greater. */
	if (v >= 0x0210) {
		/*
		 * Find the PCI IRQ Routing table.
		 */
		pcibios_pir_init();
	}

#ifdef PCIBIOS_INTR_FIXUP
	if (pci_route_table != NULL) {
		int rv;
		u_int16_t pciirq;

		rv = pci_intr_fixup(NULL, I386_BUS_SPACE_IO, &pciirq);
		switch (rv) {
		case -1:
			/* Non-fatal error. */
			printf("Warning: unable to fix up PCI interrupt routing\n");
			break;

		case 1:
			/* Fatal error. */
			panic("pcibios_init: interrupt fixup failed");
			break;
		}
	}
#endif

#ifdef PCIBIOS_BUS_FIXUP
	pcibios_max_bus = pci_bus_fixup(NULL, 0);
#ifdef PCIBIOSVERBOSE
	printf("PCI bus #%d is the last bus\n", pcibios_max_bus);
#endif
#endif

#ifdef PCIBIOS_ADDR_FIXUP
	pci_addr_fixup(NULL, pcibios_max_bus);
#endif
}

/*
 * Look for the interrupt routing table.
 *
 * We use PCI BIOS's PIR table if it's available. $PIR is the standard way
 * to do this.  Sadly, some machines are not standards conforming and have
 * _PIR instead.  We shrug and cope by looking for both.
 */
void
pcibios_pir_init(void)
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
