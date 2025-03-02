/*	$NetBSD: pci_machdep.c,v 1.28.8.1 1998/08/13 18:21:41 mellon Exp $	*/

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
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-specific functions for PCI autoconfiguration.
 *
 * On PCs, there are two methods of generating PCI configuration cycles.
 * We try to detect the appropriate mechanism for this machine and set
 * up a few function pointers to access the correct method directly.
 *
 * The configuration method can be hard-coded in the config file by
 * using `options PCI_CONF_MODE=N', where `N' is the configuration mode
 * as defined section 3.6.4.1, `Generating Configuration Cycles'.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <vm/include/vm_kern.h>
#include <vm/include/vm.h>

#define _I386_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/pio.h>

#include <machine/intr.h>
#include <dev/core/isa/isavar.h>
#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcidevs.h>

#if NIOAPIC > 0
#include <machine/apic/ioapicreg.h>
#include <machine/apic/ioapicvar.h>
#include <machine/mpconfig.h>
#include <machine/pic.h>
#endif

#ifdef MPBIOS
#include <machine/mpbiosvar.h>
#endif

int pci_mode = -1;

#define	PCI_MODE1_ENABLE		0x80000000UL
#define	PCI_MODE1_ADDRESS_REG	0x0cf8
#define	PCI_MODE1_DATA_REG		0x0cfc

#define	PCI_MODE2_ENABLE_REG	0x0cf8
#define	PCI_MODE2_FORWARD_REG	0x0cfa

#define _m1tag(b, d, f) 				\
	(PCI_MODE1_ENABLE | ((b) << 16) | ((d) << 11) | ((f) << 8))
#define _id(v, p) 						\
	(((v) << PCI_VENDOR_SHIFT) | ((p) << PCI_PRODUCT_SHIFT))
#define _qe(bus, dev, fcn, vend, prod) \
	{_m1tag(bus, dev, fcn), _id(vend, prod)}
struct {
	u_int32_t tag;
	pcireg_t id;
} pcim1_quirk_tbl[] = {
	_qe(0, 0, 0, PCI_VENDOR_COMPAQ, PCI_PRODUCT_COMPAQ_TRIFLEX1),
	/* XXX Triflex2 not tested */
	_qe(0, 0, 0, PCI_VENDOR_COMPAQ, PCI_PRODUCT_COMPAQ_TRIFLEX2),
	_qe(0, 0, 0, PCI_VENDOR_COMPAQ, PCI_PRODUCT_COMPAQ_TRIFLEX4),
	/* Triton needed for Connectix Virtual PC */
	_qe(0, 0, 0, PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82437FX),
	{0, 0xffffffff} /* patchable */
};
#undef _m1tag
#undef _id
#undef _qe

struct pci_bridge_hook_arg {
	func_t	func;
	void 	*arg;
};

static void pci_bridge_hook(pci_chipset_tag_t, pcitag_t, void *);

/*
 * PCI doesn't have any special needs; just use the generic versions
 * of these functions.
 */
struct i386_bus_dma_tag pci_bus_dma_tag = {
	NULL,					/* _cookie */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,					/* _dmamap_sync */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dmamem_alloc_range
};

void
pci_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
	if (pba->pba_bus == 0)
		printf(": configuration mode %d", pci_mode);

#ifdef MPBIOS
	mpbios_pci_attach_hook(parent, self, pba);
#endif
}

int
pci_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{
	/*
	 * Bus number is irrelevant.  If Configuration Mechanism 2 is in
	 * use, can only have devices 0-15 on any bus.  If Configuration
	 * Mechanism 1 is in use, can have devices 0-32 (i.e. the `normal'
	 * range).
	 */
	if (pci_mode == 2) {
		return (16);
	} else {
		return (32);
	}
}

pcitag_t
pci_make_tag(pc, bus, device, function)
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	pcitag_t tag;

#ifndef PCI_CONF_MODE
	switch (pci_mode) {
	case 1:
		goto mode1;
	case 2:
		goto mode2;
	default:
		panic("pci_make_tag: mode not configured");
	}
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
#ifndef PCI_CONF_MODE
mode1:
#endif
	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	tag.mode1 = PCI_MODE1_ENABLE |
		    (bus << 16) | (device << 11) | (function << 8);
	return tag;
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 2)
#ifndef PCI_CONF_MODE
mode2:
#endif
	if (bus >= 256 || device >= 16 || function >= 8)
		panic("pci_make_tag: bad request");

	tag.mode2.port = 0xc000 | (device << 8);
	tag.mode2.enable = 0xf0 | (function << 1);
	tag.mode2.forward = bus;
	return tag;
#endif
}

void
pci_decompose_tag(pc, tag, bp, dp, fp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int *bp, *dp, *fp;
{

#ifndef PCI_CONF_MODE
	switch (pci_mode) {
	case 1:
		goto mode1;
	case 2:
		goto mode2;
	default:
		panic("pci_decompose_tag: mode not configured");
	}
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
#ifndef PCI_CONF_MODE
mode1:
#endif
	if (bp != NULL)
		*bp = (tag.mode1 >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag.mode1 >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag.mode1 >> 8) & 0x7;
	return;
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 2)
#ifndef PCI_CONF_MODE
mode2:
#endif
	if (bp != NULL)
		*bp = tag.mode2.forward & 0xff;
	if (dp != NULL)
		*dp = (tag.mode2.port >> 8) & 0xf;
	if (fp != NULL)
		*fp = (tag.mode2.enable >> 1) & 0x7;
#endif
}

pcireg_t
pci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	pcireg_t data;

#ifndef PCI_CONF_MODE
	switch (pci_mode) {
	case 1:
		goto mode1;
	case 2:
		goto mode2;
	default:
		panic("pci_conf_read: mode not configured");
	}
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
#ifndef PCI_CONF_MODE
mode1:
#endif
	outl(PCI_MODE1_ADDRESS_REG, tag.mode1 | reg);
	data = inl(PCI_MODE1_DATA_REG);
	outl(PCI_MODE1_ADDRESS_REG, 0);
	return data;
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 2)
#ifndef PCI_CONF_MODE
mode2:
#endif
	outb(PCI_MODE2_ENABLE_REG, tag.mode2.enable);
	outb(PCI_MODE2_FORWARD_REG, tag.mode2.forward);
	data = inl(tag.mode2.port | reg);
	outb(PCI_MODE2_ENABLE_REG, 0);
	return data;
#endif
}

void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{

#ifndef PCI_CONF_MODE
	switch (pci_mode) {
	case 1:
		goto mode1;
	case 2:
		goto mode2;
	default:
		panic("pci_conf_write: mode not configured");
	}
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
#ifndef PCI_CONF_MODE
mode1:
#endif
	outl(PCI_MODE1_ADDRESS_REG, tag.mode1 | reg);
	outl(PCI_MODE1_DATA_REG, data);
	outl(PCI_MODE1_ADDRESS_REG, 0);
	return;
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 2)
#ifndef PCI_CONF_MODE
mode2:
#endif
	outb(PCI_MODE2_ENABLE_REG, tag.mode2.enable);
	outb(PCI_MODE2_FORWARD_REG, tag.mode2.forward);
	outl(tag.mode2.port | reg, data);
	outb(PCI_MODE2_ENABLE_REG, 0);
#endif
}

void
pci_mode_set(int mode)
{
	KASSERT(pci_mode == -1 || pci_mode == mode);

	pci_mode = mode;
}

int
pci_mode_detect(void)
{
#ifdef PCI_CONF_MODE
#if (PCI_CONF_MODE == 1) || (PCI_CONF_MODE == 2)
	return (pci_mode = PCI_CONF_MODE);
#else
#error Invalid PCI configuration mode.
#endif
#else
	u_int32_t sav, val;
	int i;
	pcireg_t idreg;

	if (pci_mode != -1) {
		return pci_mode;
	}

	/*
	 * We try to divine which configuration mode the host bridge wants.
	 */

	sav = inl(PCI_MODE1_ADDRESS_REG);

	pci_mode = 1; /* assume this for now */
	/*
	 * catch some known buggy implementations of mode 1
	 */
	for (i = 0; i < sizeof(pcim1_quirk_tbl) / sizeof(pcim1_quirk_tbl[0]);
	     i++) {
		pcitag_t t;

		if (!pcim1_quirk_tbl[i].tag)
			break;
		t.mode1 = pcim1_quirk_tbl[i].tag;
		idreg = pci_conf_read(0, t, PCI_ID_REG); /* needs "pci_mode" */
		if (idreg == pcim1_quirk_tbl[i].id) {
#ifdef DEBUG
			printf("known mode 1 PCI chipset (%08x)\n", idreg);
#endif
			return (pci_mode);
		}
	}

	/*
	 * Strong check for standard compliant mode 1:
	 * 1. bit 31 ("enable") can be set
	 * 2. byte/word access does not affect register
	 */
	outl(PCI_MODE1_ADDRESS_REG, PCI_MODE1_ENABLE);
	outb(PCI_MODE1_ADDRESS_REG + 3, 0);
	outw(PCI_MODE1_ADDRESS_REG + 2, 0);
	val = inl(PCI_MODE1_ADDRESS_REG);
	if ((val & 0x80fffffc) != PCI_MODE1_ENABLE) {
#ifdef DEBUG
		printf("pci_mode_detect: mode 1 enable failed (%x)\n",
		       val);
#endif
		goto not1;
	}
	outl(PCI_MODE1_ADDRESS_REG, 0);
	val = inl(PCI_MODE1_ADDRESS_REG);
	if ((val & 0x80fffffc) != 0)
		goto not1;
	return (pci_mode);
not1:
	outl(PCI_MODE1_ADDRESS_REG, sav);

	/*
	 * This mode 2 check is quite weak (and known to give false
	 * positives on some Compaq machines).
	 * However, this doesn't matter, because this is the
	 * last test, and simply no PCI devices will be found if
	 * this happens.
	 */
	outb(PCI_MODE2_ENABLE_REG, 0);
	outb(PCI_MODE2_FORWARD_REG, 0);
	if (inb(PCI_MODE2_ENABLE_REG) != 0 ||
	    inb(PCI_MODE2_FORWARD_REG) != 0)
		goto not2;
	return (pci_mode = 2);
not2:

	return (pci_mode = 0);
#endif
}

void
pci_device_foreach(pc, maxbus, func, context)
	pci_chipset_tag_t pc;
	int maxbus;
	func_t func;
	void *context;
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
	const struct pci_quirkdata *qd;
	int bus, device, function, maxdevs, nfuncs;
	pcireg_t id, bhlcr;
	pcitag_t tag;

	for (bus = minbus; bus <= maxbus; bus++) {
		maxdevs = pci_bus_maxdevs(pc, bus);
		for (device = 0; device < maxdevs; device++) {
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
				/*
				 * XXX Not invalid, but we've done this
				 * ~forever.
				 */
				if (PCI_VENDOR(id) == 0) {
					continue;
				}

				(*func)(pc, tag, context);
			}
		}
	}
}

void
pci_bridge_foreach(pc, minbus, maxbus, func, ctx)
	pci_chipset_tag_t pc;
	int minbus, maxbus;
	func_t func;
	void *ctx;
{
	struct pci_bridge_hook_arg bridge_hook;

	bridge_hook.func = func;
	bridge_hook.arg = ctx;

	pci_device_foreach_min(pc, minbus, maxbus, pci_bridge_hook, &bridge_hook);
}

static void
pci_bridge_hook(pc, tag, ctx)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	void *ctx;
{
	struct pci_bridge_hook_arg *bridge_hook = (void *)ctx;
	pcireg_t reg;

	reg = pci_conf_read(pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(reg) == PCI_CLASS_BRIDGE &&
	    (PCI_SUBCLASS(reg) == PCI_SUBCLASS_BRIDGE_PCI ||
		PCI_SUBCLASS(reg) == PCI_SUBCLASS_BRIDGE_CARDBUS)) {
		(*bridge_hook->func)(pc, tag, bridge_hook->arg);
	}
}

int
pci_intr_map(pc, intrtag, pin, line, ihp)
	pci_chipset_tag_t pc;
	pcitag_t intrtag;
	int pin, line;
	pci_intr_handle_t *ihp;
{
#if NIOAPIC > 0
	int bus, dev, func;
#endif

	if (pin == 0) {
		/* No IRQ used. */
		goto bad;
	}

	if (pin > PCI_INTERRUPT_PIN_MAX) {
		printf("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

#if NIOAPIC > 0
	pci_decompose_tag(pc, intrtag, &bus, &dev, &func);
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(bus, (dev<<2)|(pin-1), ihp) == 0) {
			*ihp |= line;
			return 0;
		}
		/*
		 * No explicit PCI mapping found. This is not fatal,
		 * we'll try the ISA (or possibly EISA) mappings next.
		 */
	}
#endif

	/*
	 * Section 6.2.4, `Miscellaneous Functions', says that 255 means
	 * `unknown' or `no connection' on a PC.  We assume that a device with
	 * `no connection' either doesn't have an interrupt (in which case the
	 * pin number should be 0, and would have been noticed above), or
	 * wasn't configured by the BIOS (in which case we punt, since there's
	 * no real way we can know how the interrupt lines are mapped in the
	 * hardware).
	 *
	 * XXX
	 * Since IRQ 0 is only used by the clock, and we can't actually be sure
	 * that the BIOS did its job, we also recognize that as meaning that
	 * the BIOS has not configured the device.
	 */
	if (line == 0 || line == I386_PCI_INTERRUPT_LINE_NO_CONNECTION) {
		printf("pci_intr_map: no mapping for pin %c\n", '@' + pin);
		goto bad;
	} else {
		if (line >= ICU_LEN) {
			printf("pci_intr_map: bad interrupt line %d\n", line);
			goto bad;
		}
		if (line == 2) {
			printf("pci_intr_map: changed line 2 to line 9\n");
			line = 9;
		}
	}

#if NIOAPIC > 0
	if (mp_busses != NULL) {
		if (intr_find_mpmapping(mp_isa_bus, line, ihp) == 0) {
			*ihp |= line;
			return 0;
		}
#if NEISA > 0
		if (intr_find_mpmapping(mp_eisa_bus, line, ihp) == 0) {
			*ihp |= line;
			return 0;
		}
#endif
		printf("pci_intr_map: bus %d dev %d func %d pin %d; line %d\n", bus,
				dev, func, pin, line);
		printf("pci_intr_map: no MP mapping found\n");
	}
#endif

	*ihp = line;
	return 0;

bad:
	*ihp = -1;
	return 1;
}

const char *
pci_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char irqstr[8];		/* 4 + 2 + NULL + sanity */

	if (ih == 0 || ih >= ICU_LEN || ih == 2)
		panic("pci_intr_string: bogus handle 0x%x\n", ih);

#if NIOAPIC > 0
	apic_intr_string(&irqstr, pc, ih);
#else
	sprintf(irqstr, "irq %d", ih);
#endif
	return (irqstr);
}

void *
pci_intr_establish(pc, ih, level, func, arg)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	int level, (*func)(void *);
	void *arg;
{
	if (ih == 0 || ih >= ICU_LEN || ih == 2) {
		panic("pci_intr_establish: bogus handle 0x%x\n", ih);
	}

	return (isa_intr_establish(NULL, ih, IST_LEVEL, level, func, arg));
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	return (isa_intr_disestablish(NULL, cookie));
}
