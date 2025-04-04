/*	$NetBSD: vga_pci.c,v 1.23 2003/06/29 22:30:29 fvdl Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcidevs.h>
#include <dev/core/pci/pciio.h>

#include <dev/core/ic/mc6845reg.h>
#include <dev/misc/pccons/pcdisplayvar.h>
#include <dev/video/vga/vgareg.h>
#include <dev/video/vga/vgavar.h>
#include <dev/video/vga/vga_pcivar.h>

#include <dev/core/isa/isareg.h>	/* For legacy VGA address ranges */

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wsdisplayvar.h>

#include "opt_vga.h"

#define	NBARS		6	/* number of PCI BARs */

struct vga_bar {
	bus_addr_t 		vb_base;
	bus_size_t 		vb_size;
	pcireg_t 		vb_type;
	int 			vb_flags;
};

struct vga_pci_softc {
	struct vga_softc 	sc_vga;

	pci_chipset_tag_t 	sc_pc;
	pcitag_t 			sc_pcitag;

	struct vga_bar 		sc_bars[NBARS];
	struct vga_bar 		sc_rom;
};

int		vga_pci_match(struct device *, struct cfdata *, void *);
void	vga_pci_attach(struct device *, struct device *, void *);
static int vga_pci_lookup_quirks(struct pci_attach_args *);

extern struct cfdriver vga_cd;
CFOPS_DECL(vga_pci, vga_pci_match, vga_pci_attach, NULL, NULL);
CFATTACH_DECL(vga_pci, &vga_cd, &vga_pci_cops, sizeof(struct vga_pci_softc));

int		vga_pci_ioctl(void *, u_long, caddr_t, int, struct proc *);
caddr_t	vga_pci_mmap(void *, off_t, int);

const struct vga_funcs vga_pci_funcs = {
	vga_pci_ioctl,
	vga_pci_mmap,
};

static const struct {
	int id;
	int quirks;
} vga_pci_quirks[] = {
	{ PCI_ID_CODE(PCI_VENDOR_SILMOTION, PCI_PRODUCT_SILMOTION_SM712),
	 VGA_QUIRK_NOFASTSCROLL },
};

static const struct {
	int vid;
	int quirks;
} vga_pci_vquirks[] = {
	{PCI_VENDOR_ATI, VGA_QUIRK_ONEFONT},
};

static int
vga_pci_lookup_quirks(pa)
	struct pci_attach_args *pa;
{
	int i;

	for (i = 0; i < sizeof(vga_pci_quirks) / sizeof (vga_pci_quirks[0]); i++) {
		if (vga_pci_quirks[i].id == pa->pa_id)
			return (vga_pci_quirks[i].quirks);
	}
	for (i = 0; i < sizeof(vga_pci_vquirks) / sizeof (vga_pci_vquirks[0]); i++) {
		if (vga_pci_vquirks[i].vid == PCI_VENDOR(pa->pa_id))
			return (vga_pci_vquirks[i].quirks);
	}
	return (0);
}

int
vga_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	int potential;

	potential = 0;

	/*
	 * If it's prehistoric/vga or display/vga, we might match.
	 * For the console device, this is jut a sanity check.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_PREHISTORIC &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_PREHISTORIC_VGA)
		potential = 1;
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
	     PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_DISPLAY_VGA)
		potential = 1;

	if (!potential)
		return (0);

	/* check whether it is disabled by firmware */
	if ((pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG)
	    & (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE))
	    != (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE))
		return (0);

	/* If it's the console, we have a winner! */
	if (vga_is_console(pa->pa_iot, WSDISPLAY_TYPE_PCIVGA))
		return (1);

	/*
	 * If we might match, make sure that the card actually looks OK.
	 */
	if (!vga_common_probe(pa->pa_iot, pa->pa_memt))
		return (0);

	return (1);
}

void
vga_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct vga_pci_softc *psc = (struct vga_pci_softc *)self;
	struct vga_softc *sc = &psc->sc_vga;
	char devinfo[256];
	int bar, reg;

	psc->sc_pc = pa->pa_pc;
	psc->sc_pcitag = pa->pa_tag;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	/*
	 * Gather info about all the BARs.  These are used to allow
	 * the X server to map the VGA device.
	 */
	for (bar = 0; bar < NBARS; bar++) {
		reg = PCI_MAPREG_START + (bar * 4);
		psc->sc_bars[bar].vb_type = pci_mapreg_type(psc->sc_pc, psc->sc_pcitag, reg);
		if (!pci_mapreg_probe(psc->sc_pc, psc->sc_pcitag, reg, &psc->sc_bars[bar].vb_type)) {
			/* there is no valid mapping register */
			continue;
		}
		if (PCI_MAPREG_TYPE(psc->sc_bars[bar].vb_type) == PCI_MAPREG_TYPE_IO) {
			/* Don't bother fetching I/O BARs. */
			continue;
		}
		if (PCI_MAPREG_MEM_TYPE(psc->sc_bars[bar].vb_type) == PCI_MAPREG_MEM_TYPE_64BIT) {
			/* XXX */
			printf("%s: WARNING: ignoring 64-bit BAR @ 0x%02x\n", sc->sc_dev.dv_xname, reg);
			bar++;
			continue;
		}
		if (pci_mapreg_info(psc->sc_pc, psc->sc_pcitag, reg, psc->sc_bars[bar].vb_type, &psc->sc_bars[bar].vb_base, &psc->sc_bars[bar].vb_size, &psc->sc_bars[bar].vb_flags)) {
			printf("%s: WARNING: strange BAR @ 0x%02x\n", sc->sc_dev.dv_xname, reg);
		}
	}


	/* XXX Expansion ROM? */

	vga_common_attach(sc, pa->pa_iot, pa->pa_memt, WSDISPLAY_TYPE_PCIVGA, vga_pci_lookup_quirks(pa), &vga_pci_funcs);
}

int
vga_pci_cnattach(iot, memt, pc, bus, device, function)
	bus_space_tag_t iot, memt;
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	return (vga_cnattach(iot, memt, WSDISPLAY_TYPE_PCIVGA, 0));
}

int
vga_pci_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct vga_config *vc = v;
	struct vga_pci_softc *psc = (void *) vc->softc;

	switch (cmd) {
	/* PCI config read/write passthrough. */
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return (pci_devioctl(psc->sc_pc, psc->sc_pcitag, cmd, data, flag, p));

	default:
		return (ENOIOCTL);
	}
}

caddr_t
vga_pci_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct vga_config *vc = v;
	struct vga_pci_softc *psc = (void *) vc->softc;
	struct vga_bar *vb;
	int bar;

	for (bar = 0; bar < NBARS; bar++) {
		vb = &psc->sc_bars[bar];
		if (vb->vb_size == 0)
			continue;
		if (offset >= vb->vb_base && offset < (vb->vb_base + vb->vb_size)) {
			/* XXX This the right thing to do with flags? */
			return (bus_space_mmap(vc->hdl.vh_memt, vb->vb_base,
					(offset - vb->vb_base), prot, vb->vb_flags));
		}
	}

	/* XXX Expansion ROM? */

	/*
	 * Allow mmap access to the legacy ISA hole.  This is where
	 * the legacy video BIOS will be located, and also where
	 * the legacy VGA display buffer is located.
	 *
	 * XXX Security implications, here?
	 */
	if (offset >= IOM_BEGIN && offset < IOM_END)
		return (bus_space_mmap(vc->hdl.vh_memt, IOM_BEGIN, (offset - IOM_BEGIN),
				prot, 0));

	/* Range not found. */
	return ((caddr_t)-1);
}
