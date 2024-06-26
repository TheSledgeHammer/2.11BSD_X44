/*	$NetBSD: pci_addr_fixup.c,v 1.12 2003/02/26 22:23:07 fvdl Exp $	*/

/*-
 * Copyright (c) 2000 UCHIYAMA Yasushi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_addr_fixup.c,v 1.12 2003/02/26 22:23:07 fvdl Exp $");

#include "opt_pcibios.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <machine/bus.h>

#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcidevs.h>

#include <i386/pci/pcibios.h>
#include <i386/pci/pci_addr_fixup.h>

struct pciaddr pciaddr;

void	pciaddr_resource_reserve(pci_chipset_tag_t, pcitag_t, void *);
int		pciaddr_do_resource_reserve(pci_chipset_tag_t, pcitag_t, int, void *, int, bus_addr_t *, bus_size_t);
void	pciaddr_resource_allocate(pci_chipset_tag_t, pcitag_t, void *);
int		pciaddr_do_resource_allocate(pci_chipset_tag_t, pcitag_t, int, void *, int,	bus_addr_t *, bus_size_t);
int		device_is_agp(pci_chipset_tag_t, pcitag_t);

#define PCIADDR_MEM_START		0x0
#define PCIADDR_MEM_END			0xffffffff
#define PCIADDR_PORT_START		0x0
#define PCIADDR_PORT_END		0xffff

/* for ISA devices */
#define PCIADDR_ISAPORT_RESERVE	0x5800 /* empirical value */
#define PCIADDR_ISAMEM_RESERVE	(16 * 1024 * 1024)

void
pci_addr_fixup(pc, maxbus)
	pci_chipset_tag_t pc;
	int maxbus;
{
	extern vm_offset_t avail_end;
#ifdef PCIBIOSVERBOSE
	const char *verbose_header = 
		"[%s]-----------------------\n"
		"  device vendor product\n"
		"  register space address    size\n"
		"--------------------------------------------\n";
	const char *verbose_footer = 
		"--------------------------[%3d devices bogus]\n";
#endif
	const struct {
		bus_addr_t start;
		bus_size_t size;
		char *name;
	} system_reserve [] = {
		{ 0xfec00000, 0x100000, "I/O APIC" },
		{ 0xfee00000, 0x100000, "Local APIC" },
		{ 0xfffe0000, 0x20000, "BIOS PROM" },
		{ 0, 0, 0 }, /* terminator */
	}, *srp;
	vm_offset_t start;
	int error;

	pciaddr.extent_mem = extent_create("PCI I/O memory space",
					   PCIADDR_MEM_START, 
					   PCIADDR_MEM_END,
					   M_DEVBUF, 0, 0, EX_NOWAIT);
	KASSERT(pciaddr.extent_mem);
	pciaddr.extent_port = extent_create("PCI I/O port space",
					    PCIADDR_PORT_START,
					    PCIADDR_PORT_END,
					    M_DEVBUF, 0, 0, EX_NOWAIT);
	KASSERT(pciaddr.extent_port);

	/* 
	 * 1. check & reserve system BIOS setting.
	 */
	PCIBIOS_PRINTV((verbose_header, "System BIOS Setting"));
	pci_device_foreach(pc, maxbus, pciaddr_resource_reserve, NULL);
	PCIBIOS_PRINTV((verbose_footer, pciaddr.nbogus));

	/* 
	 * 2. reserve non-PCI area.
	 */
	for (srp = system_reserve; srp->size; srp++) {
		error = extent_alloc_region(pciaddr.extent_mem, srp->start,
					    srp->size, 
					    EX_NOWAIT| EX_MALLOCOK);	
		if (error != 0) {
			printf("WARNING: can't reserve area for %s.\n",
			       srp->name);
		}
	}

	/* 
	 * 3. determine allocation space 
	 */
	start = i386_round_page(avail_end + 1);
	if (start < PCIADDR_ISAMEM_RESERVE)
		start = PCIADDR_ISAMEM_RESERVE;
	pciaddr.mem_alloc_start = (start + 0x100000 + 1) & ~(0x100000 - 1);
	pciaddr.port_alloc_start = PCIADDR_ISAPORT_RESERVE;
	PCIBIOS_PRINTV((" Physical memory end: 0x%08x\n PCI memory mapped I/O "
			"space start: 0x%08x\n", (unsigned)avail_end, 
			(unsigned)pciaddr.mem_alloc_start));

	if (pciaddr.nbogus == 0)
		return; /* no need to fixup */

	/* 
	 * 4. do fixup 
	 */
	PCIBIOS_PRINTV((verbose_header, "PCIBIOS fixup stage"));
	pciaddr.nbogus = 0;
	pci_device_foreach_min(pc, 0, maxbus, pciaddr_resource_allocate, NULL);
	PCIBIOS_PRINTV((verbose_footer, pciaddr.nbogus));

}

void
pciaddr_resource_reserve(pc, tag, context)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	void *context;
{
#ifdef PCIBIOSVERBOSE
	if (pcibiosverbose)
		pciaddr_print_devid(pc, tag);
#endif
	pciaddr_resource_manage(pc, tag,
				pciaddr_do_resource_reserve,
				&pciaddr);
}

void
pciaddr_resource_allocate(pc, tag, context)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	void *context;
{
#ifdef PCIBIOSVERBOSE
	if (pcibiosverbose)
		pciaddr_print_devid(pc, tag);
#endif
	pciaddr_resource_manage(pc, tag,
				pciaddr_do_resource_allocate,
				&pciaddr);
}

void
pciaddr_resource_manage(pc, tag, func, ctx)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	pciaddr_resource_manage_func_t func;
	void                          *ctx;
{
	pcireg_t val, mask;
	bus_addr_t addr;
	bus_size_t size;
	int error, useport, usemem, mapreg, type, reg_start, reg_end, width;

	val = pci_conf_read(pc, tag, PCI_BHLC_REG);
	switch (PCI_HDRTYPE_TYPE(val)) {
	default:
		printf("WARNING: unknown PCI device header.");
		pciaddr.nbogus++;
		return;
	case 0: 
		reg_start = PCI_MAPREG_START;
		reg_end   = PCI_MAPREG_END;
		break;
	case 1: /* PCI-PCI bridge */
		reg_start = PCI_MAPREG_START;
		reg_end   = PCI_MAPREG_PPB_END;
		break;
	case 2: /* PCI-CardBus bridge */
		reg_start = PCI_MAPREG_START;
		reg_end   = PCI_MAPREG_PCB_END;
		break;
	}
	error = useport = usemem = 0;
    
	for (mapreg = reg_start; mapreg < reg_end; mapreg += width) {
		/* inquire PCI device bus space requirement */
		val = pci_conf_read(pc, tag, mapreg);
		pci_conf_write(pc, tag, mapreg, ~0);

		mask = pci_conf_read(pc, tag, mapreg);
		pci_conf_write(pc, tag, mapreg, val);
	
		type = PCI_MAPREG_TYPE(val);
		width = 4;
		if (type == PCI_MAPREG_TYPE_MEM) {
			if (PCI_MAPREG_MEM_TYPE(val) == 
			    PCI_MAPREG_MEM_TYPE_64BIT) {
				/* XXX We could examine the upper 32 bits
				 * XXX of the BAR here, but we are totally 
				 * XXX unprepared to handle a non-zero value, 
				 * XXX either here or anywhere else in 
				 * XXX i386-land. 
				 * XXX So just arrange to not look at the
				 * XXX upper 32 bits, lest we misinterpret
				 * XXX it as a 32-bit BAR set to zero. 
				 */
			    width = 8;
			}
			size = PCI_MAPREG_MEM_SIZE(mask);
		} else {
			size = PCI_MAPREG_IO_SIZE(mask);
		}
		addr = pciaddr_ioaddr(val);
	
		if (!size) /* unused register */
			continue;

		if (type == PCI_MAPREG_TYPE_MEM)
			++usemem;
		else
			++useport;

		/* reservation/allocation phase */
		error += (*func) (pc, tag, mapreg, ctx, type, &addr, size);

		PCIBIOS_PRINTV(("\n\t%02xh %s 0x%08x 0x%08x", 
				mapreg, type ? "port" : "mem ", 
				(unsigned int)addr, (unsigned int)size));
	}
    
	/* enable/disable PCI device */
	val = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);	
	if (error == 0)
		val |= (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
			PCI_COMMAND_MASTER_ENABLE);
	else
		val &= ~(PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
			 PCI_COMMAND_MASTER_ENABLE);
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, val);
    
	if (error)
		pciaddr.nbogus++;

	PCIBIOS_PRINTV(("\n\t\t[%s]\n", error ? "NG" : "OK"));
}

int
pciaddr_do_resource_allocate(pc, tag, mapreg, ctx, type, addr, size)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	void       *ctx;
	int mapreg, type;
	bus_addr_t *addr;
	bus_size_t size;
{
 	struct pciaddr *pciaddrmap = (struct pciaddr *)ctx;
	bus_addr_t start;
	int error;
 	struct extent *ex;
 
	if (*addr) /* no need to allocate */
		return (0);

 	ex = (type == PCI_MAPREG_TYPE_MEM ?
 	      pciaddrmap->extent_mem : pciaddrmap->extent_port);

	/* XXX Don't allocate if device is AGP device to avoid conflict. */
	if (device_is_agp(pc, tag)) 
		return (0);
	
	start = (type == PCI_MAPREG_TYPE_MEM ?
		 pciaddrmap->mem_alloc_start : pciaddrmap->port_alloc_start);

	if (start < ex->ex_start || start + size - 1 >= ex->ex_end) {
		PCIBIOS_PRINTV(("No available resources. fixup failed\n"));
		return (1);
	}
	error = extent_alloc_subregion(ex, start, ex->ex_end, size,
				       size, 0,
				       EX_FAST|EX_NOWAIT|EX_MALLOCOK, addr);
	if (error) {
		PCIBIOS_PRINTV(("No available resources. fixup failed\n"));
		return (1);
	}

	/* write new address to PCI device configuration header */
	pci_conf_write(pc, tag, mapreg, *addr);
	/* check */
#ifdef PCIBIOSVERBOSE
	if (!pcibiosverbose)
#endif 
	{
		printf("pci_addr_fixup: ");
		pciaddr_print_devid(pc, tag);
	}
	if (pciaddr_ioaddr(pci_conf_read(pc, tag, mapreg)) != *addr) {
		pci_conf_write(pc, tag, mapreg, 0); /* clear */
		printf("fixup failed. (new address=%#x)\n", (unsigned)*addr);
		return (1);
	}
#ifdef PCIBIOSVERBOSE
	if (!pcibiosverbose)
#endif
		printf("new address 0x%08x\n", (unsigned)*addr);

	return (0);
}

int
pciaddr_do_resource_reserve(pc, tag, mapreg, ctx, type, addr, size)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	void          *ctx;
	int type, mapreg;
	bus_addr_t *addr;
	bus_size_t size;
{
	struct extent *ex;
	struct pciaddr *pciaddrmap = (struct pciaddr *)ctx;
	int error;

	if (*addr == 0)
		return (1);

	ex = (type == PCI_MAPREG_TYPE_MEM ?
	      pciaddrmap->extent_mem : pciaddrmap->extent_port);
	
	error = extent_alloc_region(ex, *addr, size, EX_NOWAIT| EX_MALLOCOK);
	if (error) {
		PCIBIOS_PRINTV(("Resource conflict.\n"));
		pci_conf_write(pc, tag, mapreg, 0); /* clear */
		return (1);
	}

	return (0);
}

bus_addr_t
pciaddr_ioaddr(val)
	u_int32_t val;
{
	return ((PCI_MAPREG_TYPE(val) == PCI_MAPREG_TYPE_MEM)
		? PCI_MAPREG_MEM_ADDR(val)
		: PCI_MAPREG_IO_ADDR(val));
}

void
pciaddr_print_devid(pc, tag)
	pci_chipset_tag_t pc;
	pcitag_t tag;
{
	int bus, device, function;	
	pcireg_t id;
	
	id = pci_conf_read(pc, tag, PCI_ID_REG);
	pci_decompose_tag(pc, tag, &bus, &device, &function);
	printf("%03d:%02d:%d 0x%04x 0x%04x ", bus, device, function, 
	       PCI_VENDOR(id), PCI_PRODUCT(id));
}

int
device_is_agp(pc, tag)
	pci_chipset_tag_t pc;
	pcitag_t tag;
{
	pcireg_t class, status, rval;
	int off;

	/* Check AGP device. */
	class = pci_conf_read(pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(class) == PCI_CLASS_DISPLAY) {
		status = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		if (status & PCI_STATUS_CAPLIST_SUPPORT) {
			rval = pci_conf_read(pc, tag, PCI_CAPLISTPTR_REG);
			for (off = PCI_CAPLIST_PTR(rval);
			    off != 0;
			    off = PCI_CAPLIST_NEXT(rval) ) {
				rval = pci_conf_read(pc, tag, off);
				if (PCI_CAPLIST_CAP(rval) == PCI_CAP_AGP) 
					return (1);
			}
		}
	}
	return (0);
}
