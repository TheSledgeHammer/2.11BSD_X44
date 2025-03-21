/*	$NetBSD: i82365_pci.c,v 1.17 2003/01/31 00:07:42 thorpej Exp $	*/

/*
 * Copyright (c) 1997 Marc Horowitz.  All rights reserved.
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
 *	This product includes software developed by Marc Horowitz.
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
 * XXX this driver frontend is *very* i386 dependent and should be relocated
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/user.h>

#include <dev/core/io/i82365/i82365reg.h>
#include <dev/core/io/i82365/i82365var.h>

#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcidevs.h>
#include <dev/core/io/i82365/i82365_pcivar.h>

#include <dev/core/isa/isavar.h>
#include <dev/core/io/i82365/i82365_isavar.h>

/*
 * PCI constants.
 * XXX These should be in a common file!
 */
#define	PCI_CBIO		0x10	/* Configuration Base IO Address */

int		pcic_pci_match(struct device *, struct cfdata *, void *);
void	pcic_pci_attach(struct device *, struct device *, void *);

extern struct cfdriver pcic_cd;
CFOPS_DECL(pcic_pci, pcic_pci_match, pcic_pci_attach, NULL, NULL);
CFATTACH_DECL(pcic_pci, &pcic_cd, &pcic_pci_cops, sizeof(struct pcic_softc));

static struct pcmcia_chip_functions pcic_pci_functions = {
	pcic_chip_mem_alloc,
	pcic_chip_mem_free,
	pcic_chip_mem_map,
	pcic_chip_mem_unmap,

	pcic_chip_io_alloc,
	pcic_chip_io_free,
	pcic_chip_io_map,
	pcic_chip_io_unmap,

	pcic_isa_chip_intr_establish,
	pcic_isa_chip_intr_disestablish,

	pcic_chip_socket_enable,
	pcic_chip_socket_disable,
};

static void pcic_pci_callback(struct device *);

int
pcic_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata  *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_CIRRUS:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_CIRRUS_CL_PD6729:
			break;
		default:
			return (0);
		}
		break;
	default:
		return (0);
	}
	return (1);
}

void
pcic_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcic_softc *sc = (void *) self;
	struct pcic_pci_softc *psc = (void *) self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	bus_space_tag_t memt = pa->pa_memt;
	bus_space_handle_t memh;
	char *model;

	printf(": PCMCIA controller\n");

	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0, &sc->iot, &sc->ioh,
	NULL, NULL)) {
		printf(": can't map i/o space\n");
		return;
	}

	/*
	 * XXX need some memory for mapping pcmcia cards into. Ideally, this
	 * would be completely dynamic.  Practically this doesn't work,
	 * because the extent mapper doesn't know about all the devices all
	 * the time.  With ISA we could finesse the issue by specifying the
	 * memory region in the config line.  We can't do that here, so we
	 * cheat for now. Jason Thorpe, you are my Savior, come up with a fix
	 * :-)
	 */

	/* Map mem space. */
	if (bus_space_map(memt, 0xd0000, 0x4000, 0, &memh))
		panic("pcic_isa_attach: can't map i/o space");

	sc->membase = 0xd0000;
	sc->subregionmask = (1 << (0x4000 / PCIC_MEM_PAGESIZE)) - 1;

	/* same deal for io allocation */

	sc->iobase = 0x400;
	sc->iosize = 0xbff;

	/* end XXX */

	sc->pct = (pcmcia_chipset_tag_t) & pcic_pci_functions;

	sc->memt = memt;
	sc->memh = memh;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_CIRRUS_CL_PD6729:
		model = "Cirrus Logic GD6729 PCMCIA controller";
		break;
	default:
		model = "Model unknown";
		break;
	}

	printf(": %s\n", model);

	/* Enable the card. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	pcic_attach(sc);

	/*
	 * Check to see if we're using PCI or ISA interrupts. I don't
	 * know of any i386 systems that use the 6729 in PCI interrupt
	 * mode, but maybe when the PCMCIA code runs on other platforms
	 * we'll need to fix this.
	 */
	pcic_write(&sc->handle[0], PCIC_CIRRUS_EXTENDED_INDEX,
			PCIC_CIRRUS_EXT_CONTROL_1);
	if ((pcic_read(&sc->handle[0], PCIC_CIRRUS_EXTENDED_DATA) &
	PCIC_CIRRUS_EXT_CONTROL_1_PCI_INTR_MASK)) {
		printf("%s: PCI interrupts not supported\n", sc->dev.dv_xname);
		return;
	}

	psc->intr_est = pcic_pci_machdep_intr_est(pc);
	sc->irq = -1;

#if 0
	/* Map and establish the interrupt. */
	sc->ih = pcic_pci_machdep_pcic_intr_establish(sc, pcic_intr);
	if (sc->ih == NULL) {
		printf("%s: couldn't map interrupt\n", sc->dev.dv_xname);
		return;
	}
#endif

	/*
	 * Defer configuration of children until ISA has had its chance
	 * to use up whatever IO space and IRQs it wants. XXX This will
	 * only work if ISA is attached to a pcib, AND the PCI probe finds
	 * and defers the ISA attachment before this one.
	 */
	config_defer(self, pcic_pci_callback);
	config_interrupts(self, pcic_isa_config_interrupts);
}

static void
pcic_pci_callback(self)
	struct device *self;
{
	struct pcic_softc *sc = (void *) self;

	pcic_attach_sockets(sc);
}
