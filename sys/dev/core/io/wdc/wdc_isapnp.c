/*	$NetBSD: wdc_isapnp.c,v 1.26 2004/01/03 22:56:53 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_isapnp.c,v 1.15 2001/11/15 22:56:53 09:48:10 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/core/isa/isavar.h>
#include <dev/core/isa/isadmavar.h>

#include <dev/core/isapnp/isapnpreg.h>
#include <dev/core/isapnp/isapnpvar.h>
#include <dev/core/isapnp/isapnpdevs.h>

#include <dev/disk/ata/atavar.h>
#include <dev/core/io/wdc/wdcreg.h>
#include <dev/core/io/wdc/wdcvar.h>

struct wdc_isapnp_softc {
	struct	wdc_softc 		sc_wdcdev;
	struct	wdc_channel 	*wdc_chanlist[1];
	struct	wdc_channel 	wdc_channel;
	struct	ata_queue 		wdc_chqueue;
	isa_chipset_tag_t 		sc_ic;
	void					*sc_ih;
	int						sc_drq;
};

int		wdc_isapnp_probe(struct device *, struct cfdata *, void *);
void	wdc_isapnp_attach(struct device *, struct device *, void *);

CFOPS_DECL(wdc_isapnp, wdc_isapnp_probe, wdc_isapnp_attach, NULL, NULL);
CFATTACH_DECL(wdc_isapnp, &wdc_cd, &wdc_isapnp_cops, sizeof(struct wdc_isapnp_softc));

#ifdef notyet
static void	wdc_isapnp_dma_setup (struct wdc_isapnp_softc *);
static void	wdc_isapnp_dma_start (void *, void *, size_t, int);
static void	wdc_isapnp_dma_finish (void *));
#endif

int
wdc_isapnp_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	int pri, variant;

	pri = isapnp_devmatch(aux, &isapnp_wdc_devinfo, &variant);
	if (pri && variant > 0)
		pri = 0;
	return (pri);
}

void
wdc_isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_isapnp_softc *sc = (void *)self;
	struct isapnp_attach_args *ipa = aux;
	int i;

	if (ipa->ipa_nio != 2 ||
	    ipa->ipa_nmem != 0 ||
	    ipa->ipa_nmem32 != 0 ||
	    ipa->ipa_nirq != 1 ||
	    ipa->ipa_ndrq > 1) {
		printf(": unexpected configuration\n");
		return;
	}

	if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
		printf(": couldn't map registers\n");
		return;
	}

	printf(": %s %s\n", ipa->ipa_devident, ipa->ipa_devclass);

	sc->wdc_channel.cmd_iot = ipa->ipa_iot;
	sc->wdc_channel.ctl_iot = ipa->ipa_iot;
	/*
	 * An IDE controller can feed us the regions in any order. Pass
	 * them along with the 8-byte region in sc_ad.ioh, and the other
	 * (2 byte) region in auxioh.
	 */
	if (ipa->ipa_io[0].length == 8) {
		sc->wdc_channel.cmd_baseioh = ipa->ipa_io[0].h;
		sc->wdc_channel.ctl_ioh = ipa->ipa_io[1].h;
	} else {
		sc->wdc_channel.cmd_baseioh = ipa->ipa_io[1].h;
		sc->wdc_channel.ctl_ioh = ipa->ipa_io[0].h;
	}

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(sc->wdc_channel.cmd_iot,
		    sc->wdc_channel.cmd_baseioh, i, i == 0 ? 4 : 1,
		    &sc->wdc_channel.cmd_iohs[i]) != 0) {
			printf(": couldn't subregion registers\n");
			return;
		}
	}
	sc->wdc_channel.data32iot = sc->wdc_channel.cmd_iot;
	sc->wdc_channel.data32ioh = sc->wdc_channel.cmd_iohs[0];

	sc->sc_ic = ipa->ipa_ic;
	sc->sc_ih = isa_intr_establish(ipa->ipa_ic, ipa->ipa_irq[0].num,
	    ipa->ipa_irq[0].type, IPL_BIO, wdcintr, &sc->wdc_channel);

#ifdef notyet
	if (ipa->ipa_ndrq > 0) {
		sc->sc_drq = ipa->ipa_drq[0].num;

		sc->sc_ad.cap |= WDC_CAPABILITY_DMA;
		sc->sc_ad.dma_start = &wdc_isapnp_dma_start;
		sc->sc_ad.dma_finish = &wdc_isapnp_dma_finish;
		wdc_isapnp_dma_setup(sc);
	}
#endif
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->wdc_chanlist[0] = &sc->wdc_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanlist;
	sc->sc_wdcdev.nchannels = 1;
	sc->wdc_channel.ch_channel = 0;
	sc->wdc_channel.ch_wdc = &sc->sc_wdcdev;
	sc->wdc_channel.ch_queue = &sc->wdc_chqueue;
	if (sc->wdc_channel.ch_queue == NULL) {
		printf("%s: can't allocate memory for command queue",
				sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}

	wdcattach(&sc->wdc_channel);
}

#ifdef notyet
static void
wdc_isapnp_dma_setup(sc)
	struct wdc_isapnp_softc *sc;
{

	if (isa_dmamap_create(sc->sc_ic, sc->sc_drq,
	    MAXPHYS, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW)) {
		printf("%s: can't create map for drq %d\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, sc->sc_drq);
		sc->sc_wdcdev.cap &= ~WDC_CAPABILITY_DMA;
	}
}

static void
wdc_isapnp_dma_start(scv, buf, size, read)
	void *scv, *buf;
	size_t size;
	int read;
{
	struct wdc_isapnp_softc *sc = scv;

	isa_dmastart(sc->sc_ic, sc->sc_drq, buf, size, NULL,
	    (read ? DMAMODE_READ : DMAMODE_WRITE) | DMAMODE_DEMAND,
	    BUS_DMA_NOWAIT);
}

static void
wdc_isapnp_dma_finish(scv)
	void *scv;
{
	struct wdc_isapnp_softc *sc = scv;

	isa_dmadone(sc->sc_ic, sc->sc_drq);
}
#endif
