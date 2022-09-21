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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/user.h>

#include <machine/intr.h>

#include <dev/core/isa/isadmareg.h>
#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

int 	isamatch(struct device *, struct cfdata *, void *);
void 	isaattach(struct device *, struct device *, void *);
int 	isaprint(void *, const char *);
int		isasearch(struct device *, struct cfdata *, void *);
void	isa_attach_subdevs(struct isa_softc *);
void	isa_free_subdevs(struct isa_softc *);
int		isasubmatch(struct device *, struct cfdata *, void *);

CFOPS_DECL(isa, isamatch, isaattach, NULL, NULL);
CFDRIVER_DECL(NULL, isa, DV_DULL);
CFATTACH_DECL(isa, &isa_cd, &isa_cops, sizeof(struct isa_softc));

int
isamatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isabus_attach_args *iba = aux;

	if (strcmp(iba->iba_busname, cf->cf_driver->cd_name) == 0) {
		return (0);
	} else if (isahint_match(iba, cf) == 0) {
		return (0);
	}

	/* XXX check other indicators */

	return (1);
}

void
isaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_softc *sc = (struct isa_softc *)self;
	struct isabus_attach_args *iba = (struct isabus_attach_args *)aux;

	TAILQ_INIT(&sc->sc_subdevs);

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
	if (bus_space_map(sc->sc_iot, IO_DMA1, DMA1_IOSIZE, 0, &sc->sc_dma1h)) {
		panic("isaattach: can't map DMA controller #1");
	}
	if (bus_space_map(sc->sc_iot, IO_DMA2, DMA2_IOSIZE, 0, &sc->sc_dma2h)) {
		panic("isaattach: can't map DMA controller #2");
	}
	if (bus_space_map(sc->sc_iot, IO_DMAPG, 0xf, 0, &sc->sc_dmapgh)) {
		panic("isaattach: can't map DMA page registers");
	}

	/*
	 * Map port 0x84, which causes a 1.25us delay when read.
	 * We do this now, since several drivers need it.
	 */
	if (bus_space_subregion(sc->sc_iot, sc->sc_dmapgh, 0x04, 1, &sc->sc_delaybah))
#else /* NISADMA > 0 */
	if (bus_space_map(sc->sc_iot, IO_DMAPG + 0x4, 0x1, 0, &sc->sc_delaybah))
#endif /* NISADMA > 0 */
		panic("isaattach: can't map `delay port'"); /* XXX */

	/* Attach all direct-config children. */
	isa_attach_subdevs(sc);

	config_search(isasearch, self, NULL);

	/* Detect and Attach any enabled hint-config children. */
	if (config_hint_enabled(&sc->sc_dev)) {
		isahint_attach(sc);
	}
}

void
isa_attach_subdevs(sc)
	struct isa_softc *sc;
{
	struct isa_attach_args ia;
	struct isa_subdev *is;

	if (TAILQ_EMPTY(&sc->sc_subdevs)) {
		return;
	}

	TAILQ_FOREACH(is, &sc->sc_subdevs, id_bchain) {
		ia.ia_iot = sc->sc_iot;
		ia.ia_memt = sc->sc_memt;
		ia.ia_dmat = sc->sc_dmat;
		ia.ia_ic = sc->sc_ic;

		ia.ia_iobase = is->id_iobase;
		ia.ia_iosize = is->id_iosize;
		ia.ia_maddr = is->id_maddr;
		ia.ia_msize = is->id_msize;
		ia.ia_irq = is->id_irq;
		ia.ia_irq2 = is->id_irq2;
		ia.ia_drq = is->id_drq;
		ia.ia_drq2 = is->id_drq2;
		ia.ia_aux = NULL;

		ia.ia_delaybah = is->id_delaybah;

		ia.ia_pnpname = is->id_pnpname;
		ia.ia_pnpcompatnames = is->id_pnpcompatnames;

		ia.ia_nio = is->id_nio;
		ia.ia_niomem = is->id_niomem;
		ia.ia_nirq = is->id_nirq;
		ia.ia_ndrq = is->id_ndrq;

		is->id_dev = config_found_sm(&sc->sc_dev, &ia, isaprint, isasubmatch);
	}
}

void
isa_free_subdevs(struct isa_softc *sc)
{
	struct isa_subdev *is;
	struct isa_pnpname *ipn;

#define	FREEIT(x)	if (x != NULL) free((void *)x, M_DEVBUF)

	while ((is = TAILQ_FIRST(&sc->sc_subdevs)) != NULL) {
		TAILQ_REMOVE(&sc->sc_subdevs, is, id_bchain);
		FREEIT(is->id_pnpname);
		while ((ipn = is->id_pnpcompatnames) != NULL) {
			is->id_pnpcompatnames = ipn->ipn_next;
			free(ipn->ipn_name, M_DEVBUF);
			free(ipn, M_DEVBUF);
		}
		FREEIT(is->id_iobase);
		FREEIT(is->id_iosize);
		FREEIT(is->id_irq);
		FREEIT(is->id_drq);
		free(is, M_DEVBUF);
	}

#undef FREEIT
}

int
isasubmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct isa_attach_args *ia = aux;
	int i;

	if (ia->ia_nio == 0) {
		if (cf->cf_loc[ISACF_IOBASE] != IOBASEUNK)
			return (0);
	} else {
		if (cf->cf_loc[ISACF_IOBASE] != IOBASEUNK && cf->cf_loc[ISACF_IOBASE] != ia->ia_iobase)
			return (0);
	}

	if (ia->ia_niomem == 0) {
		if (cf->cf_loc[ISACF_IOBASE] != MADDRUNK)
			return (0);
	} else {
		if (cf->cf_loc[ISACF_MADDR] != MADDRUNK && cf->cf_loc[ISACF_MADDR] != ia->ia_iosize)
			return (0);
	}

	if (ia->ia_nirq == 0) {
		if (cf->cf_loc[ISACF_IRQ] != IRQUNK)
			return (0);
	} else {
		if (cf->cf_loc[ISACF_IRQ] != IRQUNK && cf->cf_loc[ISACF_IRQ] != ia->ia_irq)
			return (0);
	}

	if (ia->ia_ndrq == 0) {
		if (cf->cf_loc[ISACF_DRQ] != DRQUNK)
			return (0);
		if (cf->cf_loc[ISACF_DRQ2] != DRQUNK)
			return (0);
	} else {
		if (cf->cf_loc[ISACF_DRQ] != DRQUNK) {
			if(cf->cf_loc[ISACF_DRQ] != ia->ia_drq) {
				return (0);
			}
			return (0);
		}
		if(cf->cf_loc[ISACF_DRQ2] != DRQUNK) {
			if(cf->cf_loc[ISACF_DRQ2] != ia->ia_drq2) {
				return (0);
			}
			return (0);
		}
		/*
		for (i = 0; i < 2; i++) {
			if (i == ia->ia_ndrq)
				break;
			if (cf->cf_loc[ISACF_DRQ + i] != DRQUNK && cf->cf_loc[ISACF_DRQ + i] != ia->ia_drq)
				return (0);
		}
		for (; i < 2; i++) {
			if (cf->cf_loc[ISACF_DRQ + i] != DRQUNK)
				return (0);
		}
		*/
	}

	return (config_match(parent, cf, aux));
}

int
isaprint(aux, isa)
	void *aux;
	const char *isa;
{
	struct isa_attach_args *ia = aux;

	if (isa != NULL) {
		struct isa_pnpname *ipn;

		if (ia->ia_pnpname != NULL) {
			printf("%s", ia->ia_pnpname);
		}
		if ((ipn = ia->ia_pnpcompatnames) != NULL) {
			printf(" (");
			for (; ipn != NULL; ipn = ipn->ipn_next) {
				printf("%s", ipn->ipn_name);
			}
			printf(")");
		}
		printf(" at %s", isa);
	}

	if (ia->ia_nio) {
		if (ia->ia_iosize) {
			printf(" port 0x%x", ia->ia_iobase);
		}
		if (ia->ia_iosize > 1) {
			printf("-0x%x", ia->ia_iobase + ia->ia_iosize - 1);
		}
	}

	if (ia->ia_niomem) {
		if (ia->ia_msize) {
			printf(" iomem 0x%x", ia->ia_maddr);
		}
		if (ia->ia_msize > 1) {
			printf("-0x%x", ia->ia_maddr + ia->ia_msize - 1);
		}
	}

	if(ia->ia_nirq) {
		if (ia->ia_irq != IRQUNK) {
			printf(" irq %d", ia->ia_irq);
		}
		if (ia->ia_irq2 != IRQUNK) {
			printf(" irq2 %d", ia->ia_irq2);
		}
	}

	if (ia->ia_ndrq) {
		if (ia->ia_drq != DRQUNK) {
			printf(" drq %d", ia->ia_drq);
		}
		if (ia->ia_drq2 != DRQUNK) {
			printf(" drq2 %d", ia->ia_drq2);
		}
	}
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

		ia.ia_nio = 1;
		ia.ia_niomem = 1;
		ia.ia_nirq = 1;
		ia.ia_ndrq = 2;

		tryagain = 0;
		if (config_match(parent, cf, &ia) > 0) {
			config_attach(parent, cf, &ia, isaprint);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}
	} while (tryagain);

	return (0);
}
