/*	$NetBSD: isa.c,v 1.97 1997/08/26 19:27:22 augustss Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Charles Hannum.  All rights reserved.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/user.h>

#include <machine/intr.h>

#include <core/isa/isadmareg.h>
#include <core/isa/isareg.h>
#include <core/isa/isavar.h>

int 	isamatch (struct device *, struct cfdata *, void *);
void 	isaattach (struct device *, struct device *, void *);
int 	isaprint (void *, const char *);
int		isasearch (struct device *, struct cfdata *, void *);

struct cfdriver isa_cd = {
	NULL, "isa", isamatch, isaattach, DV_DULL, sizeof(struct isa_softc)
};

int
isamatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isabus_attach_args *iba = aux;

	if (strcmp(iba->iba_busname, cf->cf_driver->cd_name))
		return (0);

	/* XXX check other indicators */

	return (1);
}

void
isaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_softc *sc = (struct isa_softc *)self;
	struct isabus_attach_args *iba = aux;

	isa_attach_hook(parent, self, iba);
	printf("\n");

	sc->sc_iot = iba->iba_iot;
	sc->sc_memt = iba->iba_memt;
#if NISADMA > 0
	sc->sc_dmat = iba->iba_dmat;
#endif /* NISADMA > 0 */
	sc->sc_ic = iba->iba_ic;

#if NISADMA > 0

	/*
	 * Map the registers used by the ISA DMA controller.
	 */
	if (bus_space_map(sc->sc_iot, IO_DMA1, DMA1_IOSIZE, 0, &sc->sc_dma1h))
		panic("isaattach: can't map DMA controller #1");
	if (bus_space_map(sc->sc_iot, IO_DMA2, DMA2_IOSIZE, 0, &sc->sc_dma2h))
		panic("isaattach: can't map DMA controller #2");
	if (bus_space_map(sc->sc_iot, IO_DMAPG, 0xf, 0, &sc->sc_dmapgh))
		panic("isaattach: can't map DMA page registers");

	/*
	 * Map port 0x84, which causes a 1.25us delay when read.
	 * We do this now, since several drivers need it.
	 */
	if (bus_space_subregion(sc->sc_iot, sc->sc_dmapgh, 0x04, 1, &sc->sc_delaybah))
#else /* NISADMA > 0 */
	if (bus_space_map(sc->sc_iot, IO_DMAPG + 0x4, 0x1, 0, &sc->sc_delaybah))
#endif /* NISADMA > 0 */
		panic("isaattach: can't map `delay port'"); /* XXX */

	TAILQ_INIT(&sc->sc_subdevs);
	config_search(isasearch, self, NULL);
}

int
isaprint(aux, isa)
	void *aux;
	const char *isa;
{
	struct isa_attach_args *ia = aux;

	if (ia->ia_iosize)
		printf(" port 0x%x", ia->ia_iobase);
	if (ia->ia_iosize > 1)
		printf("-0x%x", ia->ia_iobase + ia->ia_iosize - 1);
	if (ia->ia_msize)
		printf(" iomem 0x%x", ia->ia_maddr);
	if (ia->ia_msize > 1)
		printf("-0x%x", ia->ia_maddr + ia->ia_msize - 1);
	if (ia->ia_irq != IRQUNK)
		printf(" irq %d", ia->ia_irq);
	if (ia->ia_drq != DRQUNK)
		printf(" drq %d", ia->ia_drq);
	if (ia->ia_drq2 != DRQUNK)
		printf(" drq2 %d", ia->ia_drq2);
	return (UNCONF);
}

int
isasearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_softc *sc = (struct isa_softc *)parent;
	struct isa_attach_args ia;
	int tryagain;

	do {
		ia.ia_iot = sc->sc_iot;
		ia.ia_memt = sc->sc_memt;
		ia.ia_dmat = sc->sc_dmat;
		ia.ia_ic = sc->sc_ic;
		ia.ia_iobase = cf->cf_loc[ISACF_IOBASE];
		ia.ia_iosize = 0x666; /* cf->cf_iosize; */
		ia.ia_maddr = cf->cf_loc[ISACF_MADDR];
		ia.ia_msize = cf->cf_loc[ISACF_MSIZE];
		ia.ia_irq = cf->cf_loc[ISACF_IRQ] == 2 ? 9 : cf->cf_loc[ISACF_IRQ];
		ia.ia_drq = cf->cf_loc[ISACF_DRQ];
		ia.ia_drq2 = cf->cf_loc[ISACF_DRQ2];
		ia.ia_delaybah = sc->sc_delaybah;

		tryagain = 0;

		if ((*cf->cf_driver->cd_match)(parent, cf, &ia) > 0) {
			config_attach(parent, cf, &ia, isaprint);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}
	} while (tryagain);

	return (0);
}


char *
isa_intr_typename(type)
	int type;
{
	switch (type) {
	case IST_NONE:
		return ("none");
	case IST_PULSE:
		return ("pulsed");
	case IST_EDGE:
		return ("edge-triggered");
	case IST_LEVEL:
		return ("level-triggered");
	default:
		panic("isa_intr_typename: invalid type %d", type);
	}
}
