/*	$NetBSD: wdc_isa.c,v 1.40 2004/01/03 22:56:53 thorpej Exp $ */

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_isa.c,v 1.40 2004/01/03 22:56:53 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/core/isa/isavar.h>
#include <dev/core/isa/isadmavar.h>

#include <dev/core/io/wdc/wdcreg.h>
#include <dev/disk/ata/atavar.h>
#include <dev/core/io/wdc/wdcvar.h>

#define	WDC_ISA_REG_NPORTS			8
#define	WDC_ISA_AUXREG_OFFSET		0x206
#define	WDC_ISA_AUXREG_NPORTS		1 /* XXX "fdc" owns ports 0x3f7/0x377 */

/* options passed via the 'flags' config keyword */
#define WDC_OPTIONS_32				0x01 /* try to use 32bit data I/O */
#define WDC_OPTIONS_ATA_NOSTREAM	0x04
#define WDC_OPTIONS_ATAPI_NOSTREAM	0x08

struct wdc_isa_softc {
	struct	wdc_softc 		sc_wdcdev;
	struct	wdc_channel 	*wdc_chanlist[1];
	struct	wdc_channel 	wdc_channel;
	struct	ata_queue 		wdc_chqueue;
	isa_chipset_tag_t 		sc_ic;
	void					*sc_ih;
	int						sc_drq;
};

int		wdc_isa_probe(struct device *, struct cfdata *, void *);
void	wdc_isa_attach(struct device *, struct device *, void *);

CFOPS_DECL(wdc_isa, wdc_isa_probe, wdc_isa_attach, NULL, NULL);
CFATTACH_DECL(wdc_isa, &wdc_cd, &wdc_isa_cops, sizeof(struct wdc_isa_softc));

static void	wdc_isa_dma_setup(struct wdc_isa_softc *);
static int	wdc_isa_dma_init(void*, int, int, void *, size_t, int);
static void wdc_isa_dma_start(void*, int, int);
static int	wdc_isa_dma_finish(void*, int, int, int);

int
wdc_isa_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct wdc_channel ch;
	struct isa_attach_args *ia = aux;
	int result = 0, i;

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_nirq < 1)
		return (0);

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	if (ia->ia_iobase == IOBASEUNK)
		return (0);
	if (ia->ia_irq == IRQUNK)
		return (0);
	if (ia->ia_ndrq > 0 && ia->ia_drq == DRQUNK)
		ia->ia_ndrq = 0;

	memset(&ch, 0, sizeof(ch));

	ch.cmd_iot = ia->ia_iot;

	if (bus_space_map(ch.cmd_iot, ia->ia_iobase,
	    WDC_ISA_REG_NPORTS, 0, &ch.cmd_baseioh))
		goto out;

	for (i = 0; i < WDC_ISA_REG_NPORTS; i++) {
		if (bus_space_subregion(ch.cmd_iot, ch.cmd_baseioh, i,
		    i == 0 ? 4 : 1, &ch.cmd_iohs[i]) != 0)
			goto outunmap;
	}

	ch.ctl_iot = ia->ia_iot;
	if (bus_space_map(ch.ctl_iot, ia->ia_iobase +
	    WDC_ISA_AUXREG_OFFSET, WDC_ISA_AUXREG_NPORTS, 0, &ch.ctl_ioh))
		goto outunmap;

	result = wdcprobe(&ch);
	if (result) {
		ia->ia_nio = 1;
		ia->ia_iosize = WDC_ISA_REG_NPORTS;

		ia->ia_nirq = 1;

		ia->ia_niomem = 0;
	}

	bus_space_unmap(ch.ctl_iot, ch.ctl_ioh, WDC_ISA_AUXREG_NPORTS);
outunmap:
	bus_space_unmap(ch.cmd_iot, ch.cmd_baseioh, WDC_ISA_REG_NPORTS);
out:
	return (result);
}

void
wdc_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_isa_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	int wdc_cf_flags = self->dv_cfdata->cf_flags;
	int i;

	sc->wdc_channel.cmd_iot = ia->ia_iot;
	sc->wdc_channel.ctl_iot = ia->ia_iot;
	sc->sc_ic = ia->ia_ic;
	if (bus_space_map(sc->wdc_channel.cmd_iot, ia->ia_iobase,
	    WDC_ISA_REG_NPORTS, 0, &sc->wdc_channel.cmd_baseioh) ||
	    bus_space_map(sc->wdc_channel.ctl_iot,
	      ia->ia_iobase + WDC_ISA_AUXREG_OFFSET,
	      WDC_ISA_AUXREG_NPORTS, 0, &sc->wdc_channel.ctl_ioh)) {
		printf(": couldn't map registers\n");
		return;
	}

	for (i = 0; i < WDC_ISA_REG_NPORTS; i++) {
		if (bus_space_subregion(sc->wdc_channel.cmd_iot,
		      sc->wdc_channel.cmd_baseioh, i, i == 0 ? 4 : 1,
		      &sc->wdc_channel.cmd_iohs[i]) != 0) {
			printf(": couldn't subregion registers\n");
			return;
		}
	}

	sc->wdc_channel.data32iot = sc->wdc_channel.cmd_iot;
	sc->wdc_channel.data32ioh = sc->wdc_channel.cmd_iohs[0];

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE, IPL_BIO, wdcintr, &sc->wdc_channel);


	if (ia->ia_ndrq > 0 && ia->ia_drq != DRQUNK) {
		sc->sc_drq = ia->ia_drq;

		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA;
		sc->sc_wdcdev.dma_arg = sc;
		sc->sc_wdcdev.dma_init = wdc_isa_dma_init;
		sc->sc_wdcdev.dma_start = wdc_isa_dma_start;
		sc->sc_wdcdev.dma_finish = wdc_isa_dma_finish;
		wdc_isa_dma_setup(sc);
	}

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_PREATA;
	if (wdc_cf_flags & WDC_OPTIONS_32)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA32;
	if (wdc_cf_flags & WDC_OPTIONS_ATA_NOSTREAM)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_ATA_NOSTREAM;
	if (wdc_cf_flags & WDC_OPTIONS_ATAPI_NOSTREAM)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_ATAPI_NOSTREAM;

	sc->sc_wdcdev.PIO_cap = 0;
	sc->wdc_chanlist[0] = &sc->wdc_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanlist;
	sc->sc_wdcdev.nchannels = 1;
	sc->wdc_channel.ch_channel  = 0;
	sc->wdc_channel.ch_wdc = &sc->sc_wdcdev;
	sc->wdc_channel.ch_queue = &sc->wdc_chqueue;

	printf("\n");

	wdcattach(&sc->wdc_channel);
}

static void
wdc_isa_dma_setup(sc)
	struct wdc_isa_softc *sc;
{
	bus_size_t maxsize;

	if ((maxsize = isa_dmamaxsize(sc->sc_ic, sc->sc_drq)) < MAXPHYS) {
		printf("%s: max DMA size %lu is less than required %d\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, (u_long)maxsize, MAXPHYS);
		sc->sc_wdcdev.cap &= ~WDC_CAPABILITY_DMA;
		return;
	}

	if (isa_drq_alloc(sc->sc_ic, sc->sc_drq) != 0) {
		printf("%s: can't reserve drq %d\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, sc->sc_drq);
		sc->sc_wdcdev.cap &= ~WDC_CAPABILITY_DMA;
		return;
	}

	if (isa_dmamap_create(sc->sc_ic, sc->sc_drq,
	    MAXPHYS, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW)) {
		printf("%s: can't create map for drq %d\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, sc->sc_drq);
		sc->sc_wdcdev.cap &= ~WDC_CAPABILITY_DMA;
	}
}

static int
wdc_isa_dma_init(v, channel, drive, databuf, datalen, read)
	void *v;
	void *databuf;
	size_t datalen;
	int channel, drive, read;
{
	struct wdc_isa_softc *sc = v;

	isa_dmastart(sc->sc_ic, sc->sc_drq, databuf, datalen, NULL,
	    (read ? DMAMODE_READ : DMAMODE_WRITE) | DMAMODE_DEMAND, BUS_DMA_NOWAIT);
	return 0;
}

static void
wdc_isa_dma_start(v, channel, drive)
	void *v;
	int channel, drive;
{
	/* nothing to do */
}

static int
wdc_isa_dma_finish(v, channel, drive, read)
	void *v;
	int channel, drive;
	int read;
{
	struct wdc_isa_softc *sc = v;

	isa_dmadone(sc->sc_ic, sc->sc_drq);
	return 0;
}
