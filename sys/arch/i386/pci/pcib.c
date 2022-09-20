/*	$NetBSD: pcib.c,v 1.32 2003/02/26 22:23:09 fvdl Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/null.h>

#include <machine/bus.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>
#include <dev/core/pci/pcidevs.h>
#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>

int		pcibmatch (struct device *, struct cfdata *, void *);
void	pcibattach (struct device *, struct device *, void *);

CFOPS_DECL(pcib, pcibmatch, pcibattach, NULL, NULL);
CFDRIVER_DECL(NULL, pcib, DV_DULL, sizeof(struct device));
CFATTACH_DECL(pcib, pcib_cd, pcib_cops);

void	pcib_callback (void *);
int		pcib_print (void *, const char *);

int
pcibmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void /**match,*/ *aux;
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match all known PCI-ISA bridges.
	 */
	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82426EX:
		case PCI_PRODUCT_INTEL_82380AB:
			return (1);
		}
		break;

	case PCI_VENDOR_UMC:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_UMC_UM8886F:
		case PCI_PRODUCT_UMC_UM82C886:
			return (1);
		}
		break;
	case PCI_VENDOR_ALI:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_ALI_M1449:
		case PCI_PRODUCT_ALI_M1543:
			return (1);
		}
		break;
	case PCI_VENDOR_COMPAQ:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_COMPAQ_PCI_ISA_BRIDGE:
			return (1);
		}
		break;
	case PCI_VENDOR_VIATECH:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_VIATECH_VT82C570MV:
		case PCI_PRODUCT_VIATECH_VT82C586_ISA:
			return (1);
		}
		break;
	}

	return (0);
}

void
pcibattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	printf("\n");

	/*
	 * Just print out a description and set the ISA bus
	 * callback.
	 */
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo, PCI_REVISION(pa->pa_class));

	set_pci_isa_bridge_callback(pcib_callback, self);
}

void
pcib_callback(arg)
	void *arg;
{
	struct device *self = arg;
	struct isabus_attach_args iba;

	/*
	 * Attach the ISA bus behind this bridge.
	 */
	bzero(&iba, sizeof(iba));
	iba.iba_busname = "isa";
	iba.iba_iot = I386_BUS_SPACE_IO;
	iba.iba_memt = I386_BUS_SPACE_MEM;
#if NISA > 0
	iba.iba_dmat = &isa_bus_dma_tag;
#endif
	config_found(self, &iba, pcib_print);
}

int
pcib_print(aux, pnp)
	void *aux;
	const char *pnp;
{

	/* Only ISAs can attach to pcib's; easy. */
	if (pnp)
		printf("isa at %s", pnp);
	return (UNCONF);
}
