/*	$NetBSD: eisa_machdep.c,v 1.6 1997/06/06 23:12:52 thorpej Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * Machine-specific functions for EISA autoconfiguration.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/extent.h>

#define _I386_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <machine/intr.h>
#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>
#include <dev/core/eisa/eisavar.h>

#if NIOAPIC > 0
#include <machine/apic/ioapicreg.h>
#include <machine/apic/ioapicvar.h>
#include <machine/mpconfig.h>
#include <machine/pic.h>
#endif

/*
 * EISA doesn't have any special needs; just use the generic versions
 * of these funcions.
 */
struct i386_bus_dma_tag eisa_bus_dma_tag = {
	NULL,				/* _cookie */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,				/* _dmamap_sync */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dmamem_alloc_range
};

void
eisa_attach_hook(parent, self, eba)
	struct device *parent, *self;
	struct eisabus_attach_args *eba;
{
	/* Nothing to do. */
}

int
eisa_maxslots(ec)
	eisa_chipset_tag_t ec;
{
	/*
	 * Always try 16 slots.
	 */
	return (16);
}

int
eisa_intr_map(ec, irq, ihp)
	eisa_chipset_tag_t ec;
	u_int irq;
	eisa_intr_handle_t *ihp;
{

	if (irq >= ICU_LEN) {
		printf("eisa_intr_map: bad IRQ %d\n", irq);
		*ihp = -1;
		return 1;
	}
	if (irq == 2) {
		printf("eisa_intr_map: changed IRQ 2 to IRQ 9\n");
		irq = 9;
	}

#if NIOAPIC > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_eisa_bus, irq, ihp) == 0
				|| intr_find_mpmapping(mp_isa_bus, irq, ihp) == 0) {
			*ihp |= irq;
			return 0;
		} else
			printf("eisa_intr_map: no MP mapping found\n");
	}
	printf("eisa_intr_map: no MP mapping found\n");
#endif

	*ihp = irq;
	return 0;
}

const char *
eisa_intr_string(ec, ih)
	eisa_chipset_tag_t ec;
	eisa_intr_handle_t ih;
{
	static char irqstr[8];		/* 4 + 2 + NULL + sanity */

	if (ih == 0 || ih >= ICU_LEN || ih == 2)
		panic("eisa_intr_string: bogus handle 0x%x\n", ih);

#if NIOAPIC > 0
	apic_intr_string(&irqstr, ec, ih);
#else
	sprintf(irqstr, "irq %d", ih);
#endif
	return (irqstr);
}

void *
eisa_intr_establish(ec, ih, type, level, func, arg)
	eisa_chipset_tag_t ec;
	eisa_intr_handle_t ih;
	int type, level, (*func) (void *);
	void *arg;
{
	if (ih == 0 || ih >= ICU_LEN || ih == 2)
		panic("eisa_intr_establish: bogus handle 0x%x\n", ih);

	return (isa_intr_establish(NULL, ih, type, level, func, arg));
}

void
eisa_intr_disestablish(ec, cookie)
	eisa_chipset_tag_t ec;
	void *cookie;
{
	return (isa_intr_disestablish(NULL, cookie));
}

int
eisa_mem_alloc(t, size, align, boundary, cacheable, addrp, bahp)
	bus_space_tag_t t;
	bus_size_t size, align;
	bus_addr_t boundary;
	int cacheable;
	bus_addr_t *addrp;
	bus_space_handle_t *bahp;
{
	extern struct extent *iomem_ex;

	/*
	 * Allocate physical address space after the ISA hole.
	 */
	return bus_space_alloc(t, ISA_HOLE_END, iomem_ex->ex_end, size, align, boundary, cacheable, addrp, bahp);
}

void
eisa_mem_free(t, bah, size)
	bus_space_tag_t t;
	bus_space_handle_t bah;
	bus_size_t size;
{
	bus_space_free(t, bah, size);
}
