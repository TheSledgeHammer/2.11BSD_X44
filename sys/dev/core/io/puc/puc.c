/*	$NetBSD: puc.c,v 1.20 2004/02/03 19:51:39 fredb Exp $	*/

/*
 * Copyright (c) 1996, 1998, 1999
 *	Christopher G. Demetriou.  All rights reserved.
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
 * PCI "universal" communication card device driver, glues com, lpt,
 * and similar ports to PCI via bridge chip often much larger than
 * the devices being glued.
 *
 * Author: Christopher G. Demetriou, May 14, 1998 (derived from NetBSD
 * sys/dev/pci/pciide.c, revision 1.6).
 *
 * These devices could be (and some times are) described as
 * communications/{serial,parallel}, etc. devices with known
 * programming interfaces, but those programming interfaces (in
 * particular the BAR assignments for devices, etc.) in fact are not
 * particularly well defined.
 *
 * After I/we have seen more of these devices, it may be possible
 * to generalize some of these bits.  In particular, devices which
 * describe themselves as communications/serial/16[45]50, and
 * communications/parallel/??? might be attached via direct
 * 'com' and 'lpt' attachments to pci.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puc.c,v 1.20 2004/02/03 19:51:39 fredb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <dev/core/io/puc/pucvar.h>
#include <sys/termios.h>
#include <dev/core/io/com/comreg.h>
#include <dev/core/io/com/comvar.h>

#include "locators.h"
#include "opt_puccn.h"

struct puc_softc {
	struct device		sc_dev;

	/* static configuration data */
	const struct puc_device_description *sc_desc;

	/* card-global dynamic data */
	void			*sc_ih;
	struct {
		int		mapped;
		bus_addr_t	a;
		bus_size_t	s;
		bus_space_tag_t	t;
		bus_space_handle_t h;
	} sc_bar_mappings[6];				/* XXX constant */

	/* per-port dynamic data */
	struct {
		struct device *dev;

		/* filled in by port attachments */
		int (*ihand)(void*);
		void *ihandarg;
	} sc_ports[PUC_MAX_PORTS];
};

int		puc_match(struct device *, struct cfdata *, void *);
void	puc_attach(struct device *, struct device *, void *);
int		puc_print(void *, const char *);
int		puc_submatch(struct device *, struct cfdata *, void *);

CFOPS_DECL(puc, puc_match, puc_attach, NULL, NULL);
CFDRIVER_DECL(NULL, puc, DV_DULL);
CFATTACH_DECL(puc, &puc_cd, &puc_cops, sizeof(struct puc_softc));

const struct puc_device_description *
	puc_find_description(pcireg_t, pcireg_t, pcireg_t, pcireg_t);
static const char *
	puc_port_type_name(int);

int
puc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	const struct puc_device_description *desc;
	pcireg_t bhlc, subsys;

	bhlc = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_BHLC_REG);
	if (PCI_HDRTYPE(bhlc) != 0)
		return (0);

	subsys = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	desc = puc_find_description(PCI_VENDOR(pa->pa_id),
	    PCI_PRODUCT(pa->pa_id), PCI_VENDOR(subsys), PCI_PRODUCT(subsys));
	if (desc != NULL)
		return (10);
#if 0
	/*
	 * Match class/subclass, so we can tell people to compile kernel
	 * with options that cause this driver to spew.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_COMMUNICATIONS) {
		switch (PCI_SUBCLASS(pa->pa_class)) {
		case PCI_SUBCLASS_COMMUNICATIONS_SERIAL:
		case PCI_SUBCLASS_COMMUNICATIONS_MODEM:
			return (1);
		}
	}
#endif
	return (0);
}

void
puc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct puc_softc *sc = (struct puc_softc *)self;
	struct pci_attach_args *pa = aux;
	struct puc_attach_args paa;
	pci_intr_handle_t intrhandle;
	pcireg_t subsys;
	int i, barindex;
	bus_addr_t base;
	bus_space_tag_t tag;
#ifdef PUCCN
	bus_space_handle_t ioh;
#endif

	subsys = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	sc->sc_desc = puc_find_description(PCI_VENDOR(pa->pa_id),
	    PCI_PRODUCT(pa->pa_id), PCI_VENDOR(subsys), PCI_PRODUCT(subsys));
	if (sc->sc_desc == NULL) {
		/*
		 * This was a class/subclass match, so tell people to compile
		 * kernel with options that cause this driver to spew.
		 */
#ifdef PUC_PRINT_REGS
		printf(":\n");
		pci_conf_print(pa->pa_pc, pa->pa_tag, NULL);
#else
		printf(": unknown PCI communications device\n");
		printf("%s: compile kernel with PUC_PRINT_REGS and larger\n",
		    sc->sc_dev.dv_xname);
		printf("%s: mesage buffer (via 'options MSGBUFSIZE=...'),\n",
		    sc->sc_dev.dv_xname);
		printf("%s: and report the result with send-pr\n",
		    sc->sc_dev.dv_xname);
#endif
		return;
	}

	printf(": %s (", sc->sc_desc->name);
	for (i = 0; PUC_PORT_VALID(sc->sc_desc, i); i++)
		printf("%s%s", i ? ", " : "",
		    puc_port_type_name(sc->sc_desc->ports[i].type));
	printf(")\n");

	for (i = 0; i < 6; i++) {
		pcireg_t bar, type;

		sc->sc_bar_mappings[i].mapped = 0;

		bar = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    PCI_MAPREG_START + 4 * i);	/* XXX const */
		if (bar == 0)			/* BAR not implemented(?) */
			continue;

		type = (PCI_MAPREG_TYPE(bar) == PCI_MAPREG_TYPE_IO ?
		    PCI_MAPREG_TYPE_IO : PCI_MAPREG_MEM_TYPE(bar));

		if (type == PCI_MAPREG_TYPE_IO) {
			tag = pa->pa_iot;
			base =  PCI_MAPREG_IO_ADDR(bar);
		} else {
			tag = pa->pa_memt;
			base =  PCI_MAPREG_MEM_ADDR(bar);
		}
#ifdef PUCCN
		if (com_is_console(tag, base, &ioh)) {
			sc->sc_bar_mappings[i].mapped = 1;
			sc->sc_bar_mappings[i].a = base;
			sc->sc_bar_mappings[i].s = COM_NPORTS;
			sc->sc_bar_mappings[i].t = tag;
			sc->sc_bar_mappings[i].h = ioh;
			continue;
		}
#endif
		sc->sc_bar_mappings[i].mapped = (pci_mapreg_map(pa,
		    PCI_MAPREG_START + 4 * i, type, 0,
		    &sc->sc_bar_mappings[i].t, &sc->sc_bar_mappings[i].h,
		    &sc->sc_bar_mappings[i].a, &sc->sc_bar_mappings[i].s)
		      == 0);
		if (sc->sc_bar_mappings[i].mapped)
			continue;

		printf("%s: couldn't map BAR at offset 0x%lx\n",
		    sc->sc_dev.dv_xname, (long)(PCI_MAPREG_START + 4 * i));
	}

	/* Map interrupt. */
	if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin, pa->pa_intrline,
			&intrhandle)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	/*
	 * XXX the sub-devices establish the interrupts, for the
	 * XXX following reasons:
	 * XXX
	 * XXX    * we can't really know what IPLs they'd want
	 * XXX
	 * XXX    * the MD dispatching code can ("should") dispatch
	 * XXX      chained interrupts better than we can.
	 * XXX
	 * XXX It would be nice if we could indicate to the MD interrupt
	 * XXX handling code that the interrupt line used by the device
	 * XXX was a PCI (level triggered) interrupt.
	 * XXX
	 * XXX It's not pretty, but hey, what is?
	 */

	/* Configure each port. */
	for (i = 0; PUC_PORT_VALID(sc->sc_desc, i); i++) {
		bus_space_handle_t subregion_handle;

		/* make sure the base address register is mapped */
		barindex = PUC_PORT_BAR_INDEX(sc->sc_desc->ports[i].bar);
		if (!sc->sc_bar_mappings[barindex].mapped) {
			printf("%s: %s port uses unmapped BAR (0x%x)\n",
			    sc->sc_dev.dv_xname,
			    puc_port_type_name(sc->sc_desc->ports[i].type),
			    sc->sc_desc->ports[i].bar);
			continue;
		}

		/* set up to configure the child device */
		paa.port = i;
		paa.type = sc->sc_desc->ports[i].type;
		paa.flags = sc->sc_desc->ports[i].flags;
		paa.pc = pa->pa_pc;
		paa.tag = pa->pa_tag;
		paa.intrhandle = intrhandle;
		paa.a = sc->sc_bar_mappings[barindex].a;
		paa.t = sc->sc_bar_mappings[barindex].t;
		paa.dmat = pa->pa_dmat;
		paa.dmat64 = pa->pa_dmat64;

		if (
#ifdef PUCCN
		!com_is_console(sc->sc_bar_mappings[barindex].t,
				sc->sc_bar_mappings[barindex].a, &subregion_handle)
				&&
#endif
				bus_space_subregion(sc->sc_bar_mappings[barindex].t,
						sc->sc_bar_mappings[barindex].h,
						sc->sc_desc->ports[i].offset,
						sc->sc_bar_mappings[barindex].s
								- sc->sc_desc->ports[i].offset,
						&subregion_handle) != 0) {
			printf("%s: couldn't get subregion for port %d\n",
					sc->sc_dev.dv_xname, i);
			continue;
		}
		paa.h = subregion_handle;

#if 0
		printf("%s: port %d: %s @ (index %d) 0x%x (0x%lx, 0x%lx)\n",
		    sc->sc_dev.dv_xname, paa.port,
		    puc_port_type_name(paa.type), barindex, (int)paa.a,
		    (long)paa.t, (long)paa.h);
#endif

		/* and configure it */
		sc->sc_ports[i].dev = config_found_sm(self, &paa, puc_print,
		    puc_submatch);
	}
}

int
puc_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct puc_attach_args *paa = aux;
        
	if (pnp)
		printf("%s at %s", puc_port_type_name(paa->type), pnp);
	printf(" port %d", paa->port);
	return (UNCONF);
}

int
puc_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct puc_attach_args *aa = aux;

	if (cf->cf_loc[PUCCF_PORT] != PUCCF_PORT_DEFAULT &&
	    cf->cf_loc[PUCCF_PORT] != aa->port)
		return 0;
	return (config_match(parent, cf, aux));
}

const struct puc_device_description *
puc_find_description(vend, prod, svend, sprod)
	pcireg_t vend, prod, svend, sprod;
{
	int i;

#define checkreg(val, index) \
    (((val) & puc_devices[i].rmask[(index)]) == puc_devices[i].rval[(index)])

	for (i = 0; puc_devices[i].name != NULL; i++) {
		if (checkreg(vend, PUC_REG_VEND) &&
		    checkreg(prod, PUC_REG_PROD) &&
		    checkreg(svend, PUC_REG_SVEND) &&
		    checkreg(sprod, PUC_REG_SPROD))
			return (&puc_devices[i]);
	}

#undef checkreg

	return (NULL);
}

static const char *
puc_port_type_name(type)
	int type;
{
	switch (type) {
	case PUC_PORT_TYPE_COM:
		return "com";
	case PUC_PORT_TYPE_LPT:
		return "lpt";
	default:
		panic("puc_port_type_name %d", type);
	}
}
