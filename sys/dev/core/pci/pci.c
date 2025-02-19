/*	$NetBSD: pci.c,v 1.29 1997/08/30 06:53:57 mycroft Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1997
 *     Christopher G. Demetriou.  All rights reserved.
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
 * PCI bus autoconfiguration.
 */
#include <sys/cdefs.h>

#include "opt_pciverbose.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/user.h>
#include <sys/conf.h>

#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pciio.h>
#include <dev/core/pci/pcidevs.h>

#include "locators.h"

dev_type_open(pciopen);
dev_type_close(pciclose);
dev_type_ioctl(pciioctl);
dev_type_mmap(pcimmap);

const struct cdevsw pci_cdevsw = {
		.d_open = pciopen,
		.d_close = pciclose,
		.d_read = noread,
		.d_write = nowrite,
		.d_ioctl = pciioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = nopoll,
		.d_mmap = pcimmap,
		.d_kqfilter = nokqfilter,
		.d_type = D_TTY
};

int 	pcimatch(struct device *, struct cfdata *, void *);
void 	pciattach(struct device *, struct device *, void *);
int		pciprint(void *, const char *);
int		pcisubmatch(struct device *, struct cfdata *, void *);

CFOPS_DECL(pci, pcimatch, pciattach, NULL, NULL);
CFDRIVER_DECL(NULL, pci, DV_DULL);
CFATTACH_DECL(pci, &pci_cd, &pci_cops, sizeof(struct device));

/*
 * Callback so that ISA/EISA bridges can attach their child busses
 * after PCI configuration is done.
 *
 * This works because:
 *	(1) there can be at most one ISA/EISA bridge per PCI bus, and
 *	(2) any ISA/EISA bridges must be attached to primary PCI
 *	    busses (i.e. bus zero).
 *
 * That boils down to: there can only be one of these outstanding
 * at a time, it is cleared when configuring PCI bus 0 before any
 * subdevices have been found, and it is run after all subdevices
 * of PCI bus 0 have been found.
 *
 * This is needed because there are some (legacy) PCI devices which
 * can show up as ISA/EISA devices as well (the prime example of which
 * are VGA controllers).  If you attach ISA from a PCI-ISA/EISA bridge,
 * and the bridge is seen before the video board is, the board can show
 * up as an ISA device, and that can (bogusly) complicate the PCI device's
 * attach code, or make the PCI device not be properly attached at all.
 */
static void	(*pci_isa_bridge_callback)(void *);
static void	*pci_isa_bridge_callback_arg;

int
pcimatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcibus_attach_args *pba = aux;

	if (strcmp(pba->pba_busname, cf->cf_driver->cd_name))
		return (0);

	/* Check the locators */
	if ((cf->cf_loc[PCIBUSCF_BUS] != PCIBUS_UNK_BUS) && (cf->cf_loc[PCIBUSCF_BUS] != pba->pba_bus))
		return (0);

	/* sanity */
	if (pba->pba_bus < 0 || pba->pba_bus > 255)
		return (0);

	/*
	 * XXX check other (hardware?) indicators
	 */

	return 1;
}

void
pciattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcibus_attach_args *pba = aux;
	struct pci_softc *sc = (struct pci_softc *)self;
	bus_space_tag_t iot, memt;
	pci_chipset_tag_t pc;
	int bus, device, maxndevs, function, nfunctions;

	pci_attach_hook(parent, self, pba);
	printf("\n");

	iot = pba->pba_iot;
	memt = pba->pba_memt;
	pc = pba->pba_pc;
	bus = pba->pba_bus;
	maxndevs = pci_bus_maxdevs(pc, bus);

	if (bus == 0)
		pci_isa_bridge_callback = NULL;

	for (device = 0; device < maxndevs; device++) {
		pcitag_t tag;
		pcireg_t id, class, intr, bhlcr, csr;
		struct pci_attach_args pa;
		int pin;

		tag = pci_make_tag(pc, bus, device, 0);
		id = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == 0 || id == 0xffffffff)
			continue;

		bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
		nfunctions = PCI_HDRTYPE_MULTIFN(bhlcr) ? 8 : 1;

		for (function = 0; function < nfunctions; function++) {
			tag = pci_make_tag(pc, bus, device, function);
			id = pci_conf_read(pc, tag, PCI_ID_REG);
			if (id == 0 || id == 0xffffffff)
				continue;
			csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
			class = pci_conf_read(pc, tag, PCI_CLASS_REG);
			intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);

			pa.pa_iot = iot;
			pa.pa_memt = memt;
			pa.pa_dmat = pba->pba_dmat;
			pa.pa_pc = pc;
			pa.pa_device = device;
			pa.pa_function = function;
			pa.pa_tag = tag;
			pa.pa_id = id;
			pa.pa_class = class;

			/* set up memory and I/O enable flags as appropriate */
			pa.pa_flags = 0;
			if ((pba->pba_flags & PCI_FLAGS_IO_ENABLED) &&
			    (csr & PCI_COMMAND_IO_ENABLE))
				pa.pa_flags |= PCI_FLAGS_IO_ENABLED;
			if ((pba->pba_flags & PCI_FLAGS_MEM_ENABLED) &&
			    (csr & PCI_COMMAND_MEM_ENABLE))
				pa.pa_flags |= PCI_FLAGS_MEM_ENABLED;

			if (bus == 0) {
				pa.pa_intrswiz = 0;
				pa.pa_intrtag = tag;
			} else {
				pa.pa_intrswiz = pba->pba_intrswiz + device;
				pa.pa_intrtag = pba->pba_intrtag;
			}
			pin = PCI_INTERRUPT_PIN(intr);
			if (pin == PCI_INTERRUPT_PIN_NONE) {
				/* no interrupt */
				pa.pa_intrpin = 0;
			} else {
				/*
				 * swizzle it based on the number of
				 * busses we're behind and our device
				 * number.
				 */
				pa.pa_intrpin =			/* XXX */
				    ((pin + pa.pa_intrswiz - 1) % 4) + 1;
			}
			pa.pa_intrline = PCI_INTERRUPT_LINE(intr);

			config_found_sm(self, &pa, pciprint, pcisubmatch);
		}
	}

	if (bus == 0 && pci_isa_bridge_callback != NULL)
		(*pci_isa_bridge_callback)(pci_isa_bridge_callback_arg);
}

int
pciprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct pci_attach_args *pa = aux;
	char devinfo[256];

	if (pnp) {
		pci_devinfo(pa->pa_id, pa->pa_class, 1, devinfo);
		printf("%s at %s", devinfo, pnp);
	}
	printf(" dev %d function %d", pa->pa_device, pa->pa_function);
	if (!pnp) {
		pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
		printf(" %s", devinfo);
	}
#if 0
	printf(" (%si/o, %smem)",
	    pa->pa_flags & PCI_FLAGS_IO_ENABLED ? "" : "no ",
	    pa->pa_flags & PCI_FLAGS_MEM_ENABLED ? "" : "no ");
#endif
	return (UNCONF);
}

int
pcisubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if ((cf->cf_loc[PCICF_DEV] != PCI_UNK_DEV) && (cf->cf_loc[PCICF_DEV] != pa->pa_device))
		return 0;
	if ((cf->cf_loc[PCICF_FUNCTION] != PCI_UNK_FUNCTION) &&
	    (cf->cf_loc[PCICF_FUNCTION] != pa->pa_function))
		return 0;
	return (config_match(parent, cf, aux));
}

void
set_pci_isa_bridge_callback(fn, arg)
	void (*fn) (void *);
	void *arg;
{
	if (pci_isa_bridge_callback != NULL)
		panic("set_pci_isa_bridge_callback");
	pci_isa_bridge_callback = fn;
	pci_isa_bridge_callback_arg = arg;
}

/*
 * User -> kernel interface for PCI bus access.
 */
int
pciopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct pci_softc *sc;
	int unit;

	unit = minor(dev);
	sc = pci_cd.cd_devs[unit];
	if (sc == NULL)
		return (ENXIO);

	return (0);
}

int
pciclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{

	return (0);
}

int
pciioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct pci_softc *sc = pci_cd.cd_devs[minor(dev)];
	struct pciio_bdf_cfgreg *bdfr = (void *) data;
	struct pciio_businfo *binfo = (void *) data;
	pcitag_t tag;

	switch (cmd) {
	case PCI_IOC_BDF_CFGREAD:
	case PCI_IOC_BDF_CFGWRITE:
		if (bdfr->bus > 255 || bdfr->device >= sc->sc_maxndevs ||
		    bdfr->function > 7)
			return (EINVAL);
		tag = pci_make_tag(sc->sc_pc, bdfr->bus, bdfr->device,
		    bdfr->function);
		if (cmd == PCI_IOC_BDF_CFGREAD)
			bdfr->cfgreg.val = pci_conf_read(sc->sc_pc, tag,
			    bdfr->cfgreg.reg);
		else {
			if ((flag & FWRITE) == 0)
				return (EBADF);
			pci_conf_write(sc->sc_pc, tag, bdfr->cfgreg.reg,
			    bdfr->cfgreg.val);
		}
		break;

	case PCI_IOC_BUSINFO:
		binfo->busno = sc->sc_bus;
		binfo->maxdevs = sc->sc_maxndevs;
		break;

	default:
		return (ENOTTY);
	}

	return (0);
}

caddr_t
pcimmap(dev, offset, prot)
	dev_t dev;
	off_t offset;
	int prot;
{
#if 0
	struct pci_softc *sc = pci_cd.cd_devs[minor(dev)];

	/*
	 * Since we allow mapping of the entire bus, we
	 * take the offset to be the address on the bus,
	 * and pass 0 as the offset into that range.
	 *
	 * XXX Need a way to deal with linear/prefetchable/etc.
	 */
	return (bus_space_mmap(sc->sc_memt, offset, 0, prot, 0));
#else
	/* XXX Consider this further. */
	return ((caddr_t)-1);
#endif
}

/*
 * pci_devioctl:
 *
 *	PCI ioctls that can be performed on devices directly.
 */
int
pci_devioctl(pc, tag, cmd, data, flag, p)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	u_long cmd;
	caddr_t data;
    int flag;
    struct proc *p;
{
	struct pciio_cfgreg *r = (void *) data;

	switch (cmd) {
	case PCI_IOC_CFGREAD:
		if ((flag & FREAD) == 0) {
			return (EBADF);
		}
		r->val = pci_conf_read(pc, tag, r->reg);
		break;

	case PCI_IOC_CFGWRITE:
		if ((flag & FWRITE) == 0) {
			return (EBADF);
		}
		pci_conf_write(pc, tag, r->reg, r->val);
		break;

	default:
		return (EPASSTHROUGH);
	}
	return (0);
}

int
pci_probe_device(struct pci_softc *sc, pcitag_t tag, int (*match)(struct pci_attach_args *), struct pci_attach_args *pap)
{
	pci_chipset_tag_t pc = sc->sc_pc;
	struct pci_attach_args pa;
	pcireg_t id, csr, class, intr, bhlcr;
	int ret, pin, bus, device, function;

	pci_decompose_tag(pc, tag, &bus, &device, &function);

	bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
	if (PCI_HDRTYPE_TYPE(bhlcr) > 2)
		return (0);

	id = pci_conf_read(pc, tag, PCI_ID_REG);
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	class = pci_conf_read(pc, tag, PCI_CLASS_REG);

	/* Invalid vendor ID value? */
	if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
		return (0);
	/* XXX Not invalid, but we've done this ~forever. */
	if (PCI_VENDOR(id) == 0)
		return (0);

	pa.pa_iot = sc->sc_iot;
	pa.pa_memt = sc->sc_memt;
	pa.pa_dmat = sc->sc_dmat;
	pa.pa_dmat64 = sc->sc_dmat64;
	pa.pa_pc = pc;
	pa.pa_bus = bus;
	pa.pa_device = device;
	pa.pa_function = function;
	pa.pa_tag = tag;
	pa.pa_id = id;
	pa.pa_class = class;

	/*
	 * Set up memory, I/O enable, and PCI command flags
	 * as appropriate.
	 */
	pa.pa_flags = sc->sc_flags;
	if ((csr & PCI_COMMAND_IO_ENABLE) == 0)
		pa.pa_flags &= ~PCI_FLAGS_IO_ENABLED;
	if ((csr & PCI_COMMAND_MEM_ENABLE) == 0)
		pa.pa_flags &= ~PCI_FLAGS_MEM_ENABLED;

	/*
	 * If the cache line size is not configured, then
	 * clear the MRL/MRM/MWI command-ok flags.
	 */
	if (PCI_CACHELINE(bhlcr) == 0)
		pa.pa_flags &= ~(PCI_FLAGS_MRL_OKAY|
		    PCI_FLAGS_MRM_OKAY|PCI_FLAGS_MWI_OKAY);

	if (sc->sc_bridgetag == NULL) {
		pa.pa_intrswiz = 0;
		pa.pa_intrtag = tag;
	} else {
		pa.pa_intrswiz = sc->sc_intrswiz + device;
		pa.pa_intrtag = sc->sc_intrtag;
	}

	intr = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);

	pin = PCI_INTERRUPT_PIN(intr);
	pa.pa_rawintrpin = pin;
	if (pin == PCI_INTERRUPT_PIN_NONE) {
		/* no interrupt */
		pa.pa_intrpin = 0;
	} else {
		/*
		 * swizzle it based on the number of busses we're
		 * behind and our device number.
		 */
		pa.pa_intrpin = 	/* XXX */
		    ((pin + pa.pa_intrswiz - 1) % 4) + 1;
	}
	pa.pa_intrline = PCI_INTERRUPT_LINE(intr);

	if (match != NULL) {
		ret = (*match)(&pa);
		if (ret != 0 && pap != NULL)
			*pap = pa;
	} else {
		ret = config_found_sm(&sc->sc_dev, &pa, pciprint,
		    pcisubmatch) != NULL;
	}

	return (ret);
}
int
pci_get_capability(pc, tag, capid, offset, value)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int capid;
	int *offset;
	pcireg_t *value;
{
	pcireg_t reg;
	unsigned int ofs;

	reg = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	if (!(reg & PCI_STATUS_CAPLIST_SUPPORT))
		return (0);

	/* Determine the Capability List Pointer register to start with. */
	reg = pci_conf_read(pc, tag, PCI_BHLC_REG);
	switch (PCI_HDRTYPE_TYPE(reg)) {
	case 0:	/* standard device header */
		ofs = PCI_CAPLISTPTR_REG;
		break;
	case 2:	/* PCI-CardBus Bridge header */
		ofs = PCI_CARDBUS_CAPLISTPTR_REG;
		break;
	default:
		return (0);
	}

	ofs = PCI_CAPLIST_PTR(pci_conf_read(pc, tag, ofs));
	while (ofs != 0) {
#ifdef DIAGNOSTIC
		if ((ofs & 3) || (ofs < 0x40))
			panic("pci_get_capability");
#endif
		reg = pci_conf_read(pc, tag, ofs);
		if (PCI_CAPLIST_CAP(reg) == capid) {
			if (offset)
				*offset = ofs;
			if (value)
				*value = reg;
			return (1);
		}
		ofs = PCI_CAPLIST_NEXT(reg);
	}

	return (0);
}

int
pci_find_device(struct pci_attach_args *pa,
		int (*match)(struct pci_attach_args *))
{
	extern struct cfdriver pci_cd;
	struct device *pcidev;
	int i;

	for (i = 0; i < pci_cd.cd_ndevs; i++) {
		pcidev = pci_cd.cd_devs[i];
		if (pcidev != NULL &&
		    pci_enumerate_bus((struct pci_softc *) pcidev, match, pa) != 0)
			return (1);
	}
	return (0);
}

/*
 * Generic PCI bus enumeration routine.  Used unless machine-dependent
 * code needs to provide something else.
 */
int
pci_enumerate_bus_generic(struct pci_softc *sc, int (*match)(struct pci_attach_args *), struct pci_attach_args *pap)
{
	pci_chipset_tag_t pc = sc->sc_pc;
	int device, function, nfunctions, ret;
	const struct pci_quirkdata *qd;
	pcireg_t id, bhlcr;
	pcitag_t tag;
#ifdef __PCI_BUS_DEVORDER
	char devs[32];
	int i;
#endif

#ifdef __PCI_BUS_DEVORDER
	pci_bus_devorder(sc->sc_pc, sc->sc_bus, devs);
	for (i = 0; (device = devs[i]) < 32 && device >= 0; i++)
#else
	for (device = 0; device < sc->sc_maxndevs; device++)
#endif
	{
		tag = pci_make_tag(pc, sc->sc_bus, device, 0);

		bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
		if (PCI_HDRTYPE_TYPE(bhlcr) > 2)
			continue;

		id = pci_conf_read(pc, tag, PCI_ID_REG);

		/* Invalid vendor ID value? */
		if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
			continue;
		/* XXX Not invalid, but we've done this ~forever. */
		if (PCI_VENDOR(id) == 0)
			continue;

		qd = pci_lookup_quirkdata(PCI_VENDOR(id), PCI_PRODUCT(id));

		if (qd != NULL &&
		      (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0)
			nfunctions = 8;
		else if (qd != NULL &&
		      (qd->quirks & PCI_QUIRK_MONOFUNCTION) != 0)
			nfunctions = 1;
		else
			nfunctions = PCI_HDRTYPE_MULTIFN(bhlcr) ? 8 : 1;

		for (function = 0; function < nfunctions; function++) {
			if (qd != NULL &&
			    (qd->quirks & PCI_QUIRK_SKIP_FUNC(function)) != 0)
				continue;
			tag = pci_make_tag(pc, sc->sc_bus, device, function);
			ret = pci_probe_device(sc, tag, match, pap);
			if (match != NULL && ret != 0)
				return (ret);
		}
	}
	return (0);
}

/*
 * Power Management Capability (Rev 2.2)
 */

int
pci_set_powerstate(pci_chipset_tag_t pc, pcitag_t tag, int newstate)
{
	int offset;
	pcireg_t value, cap, now;

	if (!pci_get_capability(pc, tag, PCI_CAP_PWRMGMT, &offset, &value))
		return (EOPNOTSUPP);

	cap = value >> 16;
	value = pci_conf_read(pc, tag, offset + PCI_PMCSR);
	now    = value & PCI_PMCSR_STATE_MASK;
	value &= ~PCI_PMCSR_STATE_MASK;
	switch (newstate) {
	case PCI_PWR_D0:
		if (now == PCI_PMCSR_STATE_D0)
			return (0);
		value |= PCI_PMCSR_STATE_D0;
		break;
	case PCI_PWR_D1:
		if (now == PCI_PMCSR_STATE_D1)
			return (0);
		if (now == PCI_PMCSR_STATE_D2 || now == PCI_PMCSR_STATE_D3)
			return (EINVAL);
		if (!(cap & PCI_PMCR_D1SUPP))
			return (EOPNOTSUPP);
		value |= PCI_PMCSR_STATE_D1;
		break;
	case PCI_PWR_D2:
		if (now == PCI_PMCSR_STATE_D2)
			return (0);
		if (now == PCI_PMCSR_STATE_D3)
			return (EINVAL);
		if (!(cap & PCI_PMCR_D2SUPP))
			return (EOPNOTSUPP);
		value |= PCI_PMCSR_STATE_D2;
		break;
	case PCI_PWR_D3:
		if (now == PCI_PMCSR_STATE_D3)
			return (0);
		value |= PCI_PMCSR_STATE_D3;
		break;
	default:
		return (EINVAL);
	}
	pci_conf_write(pc, tag, offset + PCI_PMCSR, value);
	DELAY(1000);

	return (0);
}

int
pci_get_powerstate(pci_chipset_tag_t pc, pcitag_t tag)
{
	int offset;
	pcireg_t value;

	if (!pci_get_capability(pc, tag, PCI_CAP_PWRMGMT, &offset, &value))
		return (PCI_PWR_D0);
	value = pci_conf_read(pc, tag, offset + PCI_PMCSR);
	value &= PCI_PMCSR_STATE_MASK;
	switch (value) {
	case PCI_PMCSR_STATE_D0:
		return (PCI_PWR_D0);
	case PCI_PMCSR_STATE_D1:
		return (PCI_PWR_D1);
	case PCI_PMCSR_STATE_D2:
		return (PCI_PWR_D2);
	case PCI_PMCSR_STATE_D3:
		return (PCI_PWR_D3);
	}

	return (PCI_PWR_D0);
}
