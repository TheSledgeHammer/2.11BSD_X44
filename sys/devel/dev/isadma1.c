/*	$NetBSD: isadma.c,v 1.32.4.1 1998/11/23 03:12:57 cgd Exp $	*/

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
 * Device driver for the ISA on-board DMA controller.
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <vm/include/vm.h>
#include <vm/include/vm_param.h>

#include <machine/bus.h>
#include <machine/param.h>

#include <dev/core/ic/i8237reg.h>
#include <dev/core/isa/isadmareg.h>
#include <dev/core/isa/isadmavar.h>
#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

struct i386_isa_chipset {
	struct isa_softc ic_isa;
};
typedef struct i386_isa_chipset *isa_chipset_tag_t;

#define	isa_dmainit(ic, bst, dmat, d)						\
	_isa_dmainit(&(ic)->ic_isa, (bst), (dmat), (d))
#define	isa_dmacascade(ic, c)								\
	_isa_dmacascade(&(ic)->ic_isa, (c))
#define	isa_dmamaxsize(ic, c)								\
	_isa_dmamaxsize(&(ic)->ic_isa, (c))
#define	isa_dmamap_create(ic, c, s, f)						\
	_isa_dmamap_create(&(ic)->ic_isa, (c), (s), (f))
#define	isa_dmamap_destroy(ic, c)							\
	_isa_dmamap_destroy(&(ic)->ic_isa, (c))
#define	isa_dmastart(ic, c, a, n, p, f, bf)					\
	_isa_dmastart(&(ic)->ic_isa, (c), (a), (n), (p), (f), (bf))
#define	isa_dmaabort(ic, c)									\
	_isa_dmaabort(&(ic)->ic_isa, (c))
#define	isa_dmacount(ic, c)									\
	_isa_dmacount(&(ic)->ic_isa, (c))
#define	isa_dmafinished(ic, c)								\
	_isa_dmafinished(&(ic)->ic_isa, (c))
#define	isa_dmadone(ic, c)									\
	_isa_dmadone(&(ic)->ic_isa, (c))
#define	isa_dmafreeze(ic)									\
	_isa_dmafreeze(&(ic)->ic_isa)
#define	isa_dmathaw(ic)										\
	_isa_dmathaw(&(ic)->ic_isa)
#define	isa_dmamem_alloc(ic, c, s, ap, f)					\
	_isa_dmamem_alloc(&(ic)->ic_isa, (c), (s), (ap), (f))
#define	isa_dmamem_free(ic, c, a, s)						\
	_isa_dmamem_free(&(ic)->ic_isa, (c), (a), (s))
#define	isa_dmamem_map(ic, c, a, s, kp, f)					\
	_isa_dmamem_map(&(ic)->ic_isa, (c), (a), (s), (kp), (f))
#define	isa_dmamem_unmap(ic, c, k, s)						\
	_isa_dmamem_unmap(&(ic)->ic_isa, (c), (k), (s))
#define	isa_dmamem_mmap(ic, c, a, s, o, p, f)				\
	_isa_dmamem_mmap(&(ic)->ic_isa, (c), (a), (s), (o), (p), (f))
#define isa_drq_alloc(ic, c)								\
	_isa_drq_alloc(&(ic)->ic_isa, c)
#define isa_drq_free(ic, c)									\
	_isa_drq_free(&(ic)->ic_isa, c)
#define	isa_drq_isfree(ic, c)								\
	_isa_drq_isfree(&(ic)->ic_isa, (c))
#define	isa_malloc(ic, c, s, p, f)							\
	_isa_malloc(&(ic)->ic_isa, (c), (s), (p), (f))
#define	isa_free(a, p)										\
	_isa_free((a), (p))
#define	isa_mappage(m, o, p)								\
	_isa_mappage((m), (o), (p))


SIMPLEQ_HEAD(isamem_list, isa_mem) isa_mem_head = SIMPLEQ_HEAD_INITIALIZER(isa_mem_head);

static inline void
_isa_dmaunmask(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	int ochan = chan & 3;

	ISA_DMA_MASK_CLR(sc, chan);

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0) {
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, DMA1_SMSK, ochan | DMA37SM_CLEAR);
	} else {
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, DMA2_SMSK, ochan | DMA37SM_CLEAR);
	}
}

static inline void
_isa_dmamask(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	int ochan = chan & 3;

	ISA_DMA_MASK_SET(sc, chan);

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0) {
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, DMA1_SMSK, ochan | DMA37SM_SET);
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, DMA1_FFC, 0);
	} else {
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, DMA2_SMSK, ochan | DMA37SM_SET);
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, DMA2_FFC, 0);
	}
}

/*
 * _isa_dmainit(): Initialize the isa_dma_state for this chipset.
 */
static void
_isa_dmainit(sc, iot, dmat, dev)
	struct isa_softc *sc;
	bus_space_tag_t iot;
	bus_dma_tag_t dmat;
	struct device *dev;
{
	int chan;

	if (sc->sc_initialized) {
		/*
		 * Some systems may have e.g. `ofisa' (OpenFirmware
		 * configuration of ISA bus) and a regular `isa'.
		 * We allow both to call the initialization function,
		 * and take the device name from the last caller
		 * (assuming it will be the indirect ISA bus).  Since
		 * `ofisa' and `isa' are the same bus with different
		 * configuration mechanisms, the space and dma tags
		 * must be the same!
		 */
		if (sc->sc_iot != iot || sc->sc_dmat != dmat) {
			panic("isa_dmainit: inconsistent ISA tags");
		}
	} else {
		sc->sc_iot = iot;
		sc->sc_dmat = dmat;

		/*
		 * Map the registers used by the ISA DMA controller.
		 */
		if (bus_space_map(sc->sc_iot, IO_DMA1, DMA1_IOSIZE, 0, &sc->sc_dma1h))
			panic("_isa_dmainit: unable to map DMA controller #1");
		if (bus_space_map(sc->sc_iot, IO_DMA2, DMA2_IOSIZE, 0, &sc->sc_dma2h))
			panic("_isa_dmainit: unable to map DMA controller #2");
		if (bus_space_map(sc->sc_iot, IO_DMAPG, 0xf, 0, &sc->sc_dmapgh))
			panic("_isa_dmainit: unable to map DMA page registers");

		/*
		 * All 8 DMA channels start out "masked".
		 */
		sc->sc_masked = 0xff;

		/*
		 * Initialize the max transfer size for each channel, if
		 * it is not initialized already (i.e. by a bus-dependent
		 * front-end).
		 */
		for (chan = 0; chan < 8; chan++) {
			if (sc->sc_maxsize[chan] == 0)
				sc->sc_maxsize[chan] = ISA_DMA_MAXSIZE_DEFAULT(chan);
		}

		sc->sc_initialized = 1;

		/*
		 * DRQ 4 is used to chain the two 8237s together; make
		 * sure it's always cascaded, and that it will be unmasked
		 * when DMA is thawed.
		 */
		_isa_dmacascade(sc, 4);
	}
}

/*
 * isa_dmacascade(): program 8237 DMA controller channel to accept
 * external dma control by a board.
 */
static void
_isa_dmacascade(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	int ochan = chan & 3;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		return (EINVAL);
	}

	if (ISA_DRQ_ISFREE(sc, chan) == 0) {
		printf("%s: DRQ %d is not free\n", sc->sc_dev.dv_xname, chan);
		return (EAGAIN);
	}

	ISA_DRQ_ALLOC(sc, chan);

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0)
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, DMA1_MODE, ochan | DMA37MD_CASCADE);
	else
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, DMA2_MODE, ochan | DMA37MD_CASCADE);

	_isa_dmaunmask(sc, chan);
	return;
}

int
_isa_drq_alloc(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	if (ISA_DMA_DRQ_ISFREE(sc, chan) == 0)
		return EBUSY;
	ISA_DMA_DRQ_ALLOC(sc, chan);
	return 0;
}

int
_isa_drq_free(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	if (ISA_DRQ_ISFREE(sc, chan))
		return EINVAL;
	ISA_DRQ_FREE(sc, chan);
	return 0;
}

bus_size_t
_isa_dmamaxsize(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev->dv_xname, chan);
		return (0);
	}

	return (sc->sc_maxsize[chan]);
}

int
_isa_dmamap_create(sc, chan, size, flags)
	struct isa_softc *sc;
	int chan;
	bus_size_t size;
	int flags;
{
	int error;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		return (EINVAL);
	}

	if (size > sc->sc_maxsize[chan]) {
		return (EINVAL);
	}

	if (ISA_DRQ_ISFREE(sc, chan) == 0) {
		printf("%s: drq %d is not free\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmamap_create");
	}

	ISA_DRQ_ALLOC(sc, chan);

	error = bus_dmamap_create(sc->sc_dmat, size, 1, size, sc->sc_maxsize[chan], flags, &sc->sc_dmamaps[chan]);

	return (error);
}

void
_isa_dmamap_destroy(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		goto lose;
	}

	if (ISA_DRQ_ISFREE(sc, chan)) {
		printf("%s: drq %d is already free\n",
		    sc->sc_dev.dv_xname, chan);
		goto lose;
	}

	ISA_DRQ_FREE(sc, chan);

	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamaps[chan]);
	return;

 lose:
	panic("isa_dmamap_destroy");
}

/*
 * isa_dmastart(): program 8237 DMA controller channel and set it
 * in motion.
 */
int
_isa_dmastart(sc, chan, addr, nbytes, p, flags, busdmaflags)
	struct isa_softc *sc;
	int 			chan;
	void 			*addr;
	bus_size_t 		nbytes;
	struct proc 	*p;
	int 			flags;
	int 			busdmaflags;
{
	bus_dmamap_t dmam;
	bus_addr_t dmaaddr;
	int waport;
	int ochan = chan & 3;
	int error;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		goto lose;
	}

#ifdef ISADMA_DEBUG
	printf("isa_dmastart: drq %d, addr %p, nbytes 0x%lx, p %p, "
	    "flags 0x%x, dmaflags 0x%x\n",
	    chan, addr, nbytes, p, flags, busdmaflags);
#endif

	if (chan & 4) {
		if (nbytes > (1 << 17) || (nbytes & 1) || ((u_long)addr & 1)) {
			printf("%s: drq %d, nbytes 0x%lx, addr %p\n",
			    sc->sc_dev.dv_xname, chan, nbytes, addr);
			goto lose;
		}
	} else {
		if (nbytes > (1 << 16)) {
			printf("%s: drq %d, nbytes 0x%lx\n",
			    sc->sc_dev.dv_xname, chan, nbytes);
			goto lose;
		}
	}

	dmam = sc->sc_dmamaps[chan];
	if (dmam == NULL)
		panic("isa_dmastart: no DMA map for chan %d\n", chan);

	error = bus_dmamap_load(sc->sc_dmat, dmam, addr, nbytes,p, busdmaflags);
	if (error)
		return (error);

#ifdef ISADMA_DEBUG
	__asm(".globl isa_dmastart_afterload ; isa_dmastart_afterload:");
#endif

	if (flags & DMAMODE_READ) {
		bus_dmamap_sync(sc->sc_dmat, dmam, BUS_DMASYNC_PREREAD);
		sc->sc_dmareads |= (1 << chan);
	} else {
		bus_dmamap_sync(sc->sc_dmat, dmam, BUS_DMASYNC_PREWRITE);
		sc->sc_dmareads &= ~(1 << chan);
	}

	dmaaddr = dmam->dm_segs[0].ds_addr;

#ifdef ISADMA_DEBUG
	printf("     dmaaddr 0x%lx\n", dmaaddr);

	__asm(".globl isa_dmastart_aftersync ; isa_dmastart_aftersync:");
#endif

	sc->sc_dmalength[chan] = nbytes;

	_isa_dmamask(sc, chan);
	sc->sc_dmafinished &= ~(1 << chan);

	if ((chan & 4) == 0) {
		/* set dma channel mode */
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, DMA1_MODE,
		    ochan | dmamode[flags]);

		/* send start address */
		waport = DMA1_CHN(ochan);
		bus_space_write_1(sc->sc_iot, sc->sc_dmapgh,
		    dmapageport[0][ochan], (dmaaddr >> 16) & 0xff);
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, waport,
		    dmaaddr & 0xff);
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, waport,
		    (dmaaddr >> 8) & 0xff);

		/* send count */
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, waport + 1,
		    (--nbytes) & 0xff);
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, waport + 1,
		    (nbytes >> 8) & 0xff);
	} else {
		/* set dma channel mode */
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, DMA2_MODE,
		    ochan | dmamode[flags]);

		/* send start address */
		waport = DMA2_CHN(ochan);
		bus_space_write_1(sc->sc_iot, sc->sc_dmapgh,
		    dmapageport[1][ochan], (dmaaddr >> 16) & 0xff);
		dmaaddr >>= 1;
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, waport,
		    dmaaddr & 0xff);
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, waport,
		    (dmaaddr >> 8) & 0xff);

		/* send count */
		nbytes >>= 1;
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, waport + 2,
		    (--nbytes) & 0xff);
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, waport + 2,
		    (nbytes >> 8) & 0xff);
	}

	_isa_dmaunmask(sc, chan);
	return (0);

 lose:
	panic("isa_dmastart");
}

void
_isa_dmaabort(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmaabort");
	}

	_isa_dmamask(sc, chan);
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamaps[chan]);
	sc->sc_dmareads &= ~(1 << chan);
}

bus_size_t
_isa_dmacount(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	int waport;
	bus_size_t nbytes;
	int ochan = chan & 3;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmacount");
	}

	_isa_dmamask(sc, chan);

	/*
	 * We have to shift the byte count by 1.  If we're in auto-initialize
	 * mode, the count may have wrapped around to the initial value.  We
	 * can't use the TC bit to check for this case, so instead we compare
	 * against the original byte count.
	 * If we're not in auto-initialize mode, then the count will wrap to
	 * -1, so we also handle that case.
	 */
	if ((chan & 4) == 0) {
		waport = DMA1_CHN(ochan);
		nbytes = bus_space_read_1(sc->sc_iot, sc->sc_dma1h,
		    waport + 1) + 1;
		nbytes += bus_space_read_1(sc->sc_iot, sc->sc_dma1h,
		    waport + 1) << 8;
		nbytes &= 0xffff;
	} else {
		waport = DMA2_CHN(ochan);
		nbytes = bus_space_read_1(sc->sc_iot, sc->sc_dma2h,
		    waport + 2) + 1;
		nbytes += bus_space_read_1(sc->sc_iot, sc->sc_dma2h,
		    waport + 2) << 8;
		nbytes <<= 1;
		nbytes &= 0x1ffff;
	}

	if (nbytes == sc->sc_dmalength[chan])
		nbytes = 0;

	_isa_dmaunmask(sc, chan);
	return (nbytes);
}

int
_isa_dmafinished(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmafinished");
	}

	/* check that the terminal count was reached */
	if ((chan & 4) == 0)
		sc->sc_dmafinished |= bus_space_read_1(sc->sc_iot, sc->sc_dma1h,
				DMA1_SR) & 0x0f;
	else
		sc->sc_dmafinished |= (bus_space_read_1(sc->sc_iot, sc->sc_dma2h,
				DMA2_SR) & 0x0f) << 4;

	return ((sc->sc_dmafinished & (1 << chan)) != 0);
}

void
_isa_dmadone(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	bus_dmamap_t dmam;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmadone");
	}

	dmam = sc->sc_dmamaps[chan];

	_isa_dmamask(sc, chan);

	if (_isa_dmafinished(sc, chan) == 0)
		printf("%s: isa_dmadone: channel %d not finished\n",
		    sc->sc_dev.dv_xname, chan);

	bus_dmamap_sync(sc->sc_dmat, dmam,
	    (sc->sc_dmareads & (1 << chan)) ? BUS_DMASYNC_POSTREAD :
	    BUS_DMASYNC_POSTWRITE);

	bus_dmamap_unload(sc->sc_dmat, dmam);
	sc->sc_dmareads &= ~(1 << chan);
}

void
_isa_dmafreeze(sc)
	struct isa_softc *sc;
{
	int s;

	s = splhigh();

	if (sc->sc_frozen == 0) {
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, DMA1_MASK, 0x0f);
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, DMA2_MASK, 0x0f);
	}

	sc->sc_frozen++;
	if (sc->sc_frozen < 1)
		panic("_isa_dmafreeze: overflow");

	splx(s);
}

void
_isa_dmathaw(sc)
	struct isa_softc *sc;
{
	int s;

	s = splhigh();

	sc->sc_frozen--;
	if (sc->sc_frozen < 0)
		panic("_isa_dmathaw: underflow");

	if (sc->sc_frozen == 0) {
		bus_space_write_1(sc->sc_iot, sc->sc_dma1h, DMA1_MASK,
				sc->sc_masked & 0x0f);
		bus_space_write_1(sc->sc_iot, sc->sc_dma2h, DMA2_MASK,
				(sc->sc_masked >> 4) & 0x0f);
	}

	splx(s);
}

int
_isa_dmamem_alloc(sc, chan, size, addrp, flags)
	struct isa_softc *sc;
	int chan;
	bus_size_t size;
	bus_addr_t *addrp;
	int flags;
{
	bus_dma_segment_t seg;
	int error, boundary, rsegs;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmamem_alloc");
	}

	boundary = (chan & 4) ? (1 << 17) : (1 << 16);

	size = round_page(size);

	error = bus_dmamem_alloc(sc->sc_dmat, size, NBPG, boundary, &seg, 1, &rsegs, flags);
	if (error)
		return (error);

	*addrp = seg.ds_addr;
	return (0);
}

void
_isa_dmamem_free(sc, chan, addr, size)
	struct isa_softc *sc;
	int chan;
	bus_addr_t addr;
	bus_size_t size;
{
	bus_dma_segment_t seg;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmamem_free");
	}

	seg.ds_addr = addr;
	seg.ds_len = size;

	bus_dmamem_free(sc->sc_dmat, &seg, 1);
}

int
_isa_dmamem_map(sc, chan, addr, size, kvap, flags)
	struct isa_softc *sc;
	int chan;
	bus_addr_t addr;
	bus_size_t size;
	caddr_t *kvap;
	int flags;
{
	bus_dma_segment_t seg;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmamem_map");
	}

	seg.ds_addr = addr;
	seg.ds_len = size;

	return (bus_dmamem_map(sc->sc_dmat, &seg, 1, size, kvap, flags));
}

void
_isa_dmamem_unmap(sc, chan, kva, size)
	struct isa_softc *sc;
	int chan;
	caddr_t kva;
	size_t size;
{
	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmamem_unmap");
	}

	bus_dmamem_unmap(sc->sc_dmat, kva, size);
}

int
_isa_dmamem_mmap(sc, chan, addr, size, off, prot, flags)
	struct isa_softc *sc;
	int chan;
	bus_addr_t addr;
	bus_size_t size;
	int off, prot, flags;
{
	bus_dma_segment_t seg;

	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_dmamem_mmap");
	}

	if (off < 0)
		return (-1);

	seg.ds_addr = addr;
	seg.ds_len = size;

	return (bus_dmamem_mmap(sc->sc_dmat, &seg, 1, off, prot, flags));
}

int
_isa_drq_isfree(sc, chan)
	struct isa_softc *sc;
	int chan;
{
	if (chan < 0 || chan > 7) {
		printf("%s: bogus drq %d\n", sc->sc_dev.dv_xname, chan);
		panic("isa_drq_isfree");
	}
	return ISA_DRQ_ISFREE(sc, chan);
}

void *
_isa_malloc(sc, chan, size, type, flags)
	struct isa_softc *sc;
	int 			chan;
	size_t 			size;
	int 			type;
	int 			flags;
{

	struct isa_mem 	*m;
	bus_addr_t 		addr;
	caddr_t 		kva;
	int 			bflags;

	bflags = flags & M_WAITOK ? BUS_DMA_WAITOK : BUS_DMA_NOWAIT;

	if (_isa_dmamem_alloc(sc, chan, size, &addr, bflags))
		return 0;
	if (_isa_dmamem_map(sc, chan, addr, size, &kva, bflags)) {
		_isa_dmamem_free(sc, chan, addr, size);
		return 0;
	}
	m = malloc(sizeof(*m), type, flags);
	if (m == 0) {
		_isa_dmamem_unmap(sc, chan, kva, size);
		_isa_dmamem_free(sc, chan, addr, size);
		return 0;
	}

	m->isa = sc;
	m->chan = chan;
	m->size = size;
	m->addr = addr;
	m->kva = kva;
	SIMPLEQ_INSERT_HEAD(&isa_mem_head, m, next1);
	return ((void *)kva);
}

void
_isa_free(addr, type)
	void *addr;
	int type;
{
	struct isa_mem **mp, *m;
	caddr_t kva = (caddr_t)addr;

	SIMPLEQ_FOREACH(m, &isa_mem_head, next1) {
		if (m->kva != kva) {
			;
		}
	}
	if (!m) {
		printf("isa_free: freeing unallocted memory\n");
		return;
	}
	SIMPLEQ_REMOVE(&isa_mem_head, m, isa_mem, next1);
	_isa_dmamem_unmap(m->isa, m->chan, kva, m->size);
	_isa_dmamem_free(m->isa, m->chan, m->addr, m->size);
	free(m, type);
}

int
_isa_mappage(mem, off, prot)
	void *mem;
	int off;
	int prot;
{
	struct isa_mem 	*m;

	SIMPLEQ_FOREACH(m, &isa_mem_head, next1) {
		;
	}
	if (!m) {
		printf("isa_mappage: mapping unallocted memory\n");
		return (-1);
	}
	return (_isa_dmamem_mmap(m->isa, m->chan, m->addr, m->size, off, prot, BUS_DMA_WAITOK));
}
