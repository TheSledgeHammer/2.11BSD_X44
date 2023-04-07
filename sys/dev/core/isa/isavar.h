/*	$NetBSD: isavar.h,v 1.30 1997/10/14 07:15:45 sakamoto Exp $	*/

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
 * Copyright (c) 1995 Chris G. Demetriou
 * Copyright (c) 1992 Berkeley Software Design, Inc.
 * All rights reserved.
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
 *	This product includes software developed by Berkeley Software
 *	Design, Inc.
 * 4. The name of Berkeley Software Design must not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI Id: isavar.h,v 1.5 1992/12/01 18:06:00 karels Exp 
 */

#ifndef _DEV_ISA_ISAVAR_H_
#define	_DEV_ISA_ISAVAR_H_

#include <sys/queue.h>
#include <machine/bus.h>

/*
 * Definitions for ISA autoconfiguration.
 */

/* 
 * Structures and definitions needed by the machine-dependent header.
 */
struct isabus_attach_args;

#include <machine/isa/isa_machdep.h>

/*
 * ISA bus attach arguments
 */
struct isabus_attach_args {
	char					*iba_busname;	/* XXX should be common */
	bus_space_tag_t 		iba_iot;		/* isa i/o space tag */
	bus_space_tag_t 		iba_memt;		/* isa mem space tag */
	bus_dma_tag_t 			iba_dmat;		/* isa DMA tag */
	isa_chipset_tag_t 		iba_ic;
};

/*
 * ISA bus resources.
 */
struct isa_pnpname {
	struct isa_pnpname 		*ipn_next;
	char 					*ipn_name;
};

/*
 * ISA driver attach arguments
 */
struct isa_attach_args {
	bus_space_tag_t 		ia_iot;			/* isa i/o space tag */
	bus_space_tag_t 		ia_memt;		/* isa mem space tag */
	bus_dma_tag_t 			ia_dmat;		/* DMA tag */

	isa_chipset_tag_t 		ia_ic;

	int						ia_iobase;		/* base i/o address ia.addr */
	int						ia_iosize;		/* span of ports used */
	int						ia_irq;			/* interrupt request */
	int						ia_irq2;		/* second interrupt request */
	int						ia_drq;			/* DMA request */
	int						ia_drq2;		/* second DMA request */
	int						ia_maddr;		/* physical i/o mem addr */
	u_int					ia_msize;		/* size of i/o memory */
	void					*ia_aux;		/* driver specific */

	bus_space_handle_t 		ia_delaybah; 	/* i/o handle for `delay port' */

	char 					*ia_pnpname;
	struct isa_pnpname 		*ia_pnpcompatnames;

	int 					ia_nio;
	int 					ia_niomem;
	int 					ia_nirq;
	int 					ia_ndrq;
};

/*
 * Test to determine if a given call to an ISA device probe routine
 * is actually an attempt to do direct configuration.
 */
#define	ISA_DIRECT_CONFIG(ia)						\
	((ia)->ia_pnpname != NULL || (ia)->ia_pnpcompatnames != NULL)

#include "locators.h"

/*
 * Per-device ISA variables
 */
struct isa_subdev {
	struct  device 			*id_dev;		/* back pointer to generic */
	TAILQ_ENTRY(isa_subdev) id_bchain;		/* bus chain */

	int						id_iobase;		/* base i/o address */
	int						id_iosize;		/* span of ports used */
	int						id_irq;			/* interrupt request */
	int						id_irq2;		/* second interrupt request */
	int						id_drq;			/* DMA request */
	int						id_drq2;		/* second DMA request */
	int						id_maddr;		/* physical i/o mem addr */
	u_int					id_msize;		/* size of i/o memory */
	void					*id_aux;		/* driver specific */

	bus_space_handle_t 		id_delaybah; 	/* i/o handle for `delay port' */

	char 					*id_pnpname;
	struct isa_pnpname 		*id_pnpcompatnames;

	int 					id_nio;
	int 					id_niomem;
	int 					id_nirq;
	int 					id_ndrq;
};

/*
 * ISA master bus
 */
struct isa_softc {
	struct	device 			sc_dev;			/* base device */
	TAILQ_HEAD(, isa_subdev) sc_subdevs;	/* list of all children */

	bus_space_tag_t 		sc_iot;			/* isa io space tag */
	bus_space_tag_t 		sc_memt;		/* isa mem space tag */
	bus_dma_tag_t 			sc_dmat;		/* isa DMA tag */

	isa_chipset_tag_t 		sc_ic;
	int 					sc_dynamicdevs;

	/*
	 * Bitmap representing the DRQ channels available for ISA.
	 */
	int						sc_drqmap;
	/*
	 * DMA maps
	 */
	bus_space_handle_t 		sc_dma1h;		/* i/o handle for DMA controller #1 */
	bus_space_handle_t 		sc_dma2h;		/* i/o handle for DMA controller #2 */
	bus_space_handle_t 		sc_dmapgh;		/* i/o handle for DMA page registers */

	/*
	 * DMA maps used for the 8 DMA channels.
	 */
	bus_dmamap_t			sc_dmamaps[8];
	vm_size_t				sc_dmalength[8];
	bus_size_t 				sc_maxsize[8];	/* max size per channel */
	int						sc_dmareads;	/* state for isa_dmadone() */
	int						sc_dmafinished;	/* DMA completion state */
	int						sc_masked;		/* masked channels (bitmap) */
	int						sc_frozen;		/* `frozen' count */
	int						sc_initialized;	/* only initialize once... */

	/*
	 * This i/o handle is used to map port 0x84, which is
	 * read to provide a 1.25us delay.  This access handle
	 * is mapped in isaattach(), and exported to drivers
	 * via isa_attach_args.
	 */
	bus_space_handle_t   	sc_delaybah;
};

#define	IOBASEUNK			-1				/* i/o address is unknown (ISACF_PORT_DEFAULT) */
#define	MADDRUNK			-1				/* shared memory address is unknown (ISACF_IOMEM_DEFAULT) */
#define	IRQUNK				-1				/* interrupt request line is unknown (ISACF_IRQ_DEFAULT) */
#define	DRQUNK				-1				/* DMA request line is unknown (ISACF_DRQ_DEFAULT)  */

#define	ISACF_IOBASE 		0
#define	ISACF_IOSIZE 		1
#define	ISACF_MADDR 		2
#define	ISACF_MSIZE 		3
#define	ISACF_IRQ			4
#define	ISACF_DRQ			5
#define	ISACF_DRQ2 			6

#define	ISA_DRQ_ISFREE(isadev, drq) 	\
	((((struct isa_softc *)(isadev))->sc_drqmap & (1 << (drq))) == 0)

#define	ISA_DRQ_ALLOC(isadev, drq) 		\
	((struct isa_softc *)(isadev))->sc_drqmap |= (1 << (drq))

#define	ISA_DRQ_FREE(isadev, drq) 		\
	((struct isa_softc *)(isadev))->sc_drqmap &= ~(1 << (drq))

#define	ISA_DMA_MASK_SET(isadev, drq)	\
	((struct isa_softc *)(isadev))->sc_masked |= (1 << (drq))

#define	ISA_DMA_MASK_CLR(isadev, drq)	\
	((struct isa_softc *)(isadev))->sc_masked &= ~(1 << (drq))

/*
 * ISA interrupt handler manipulation.
 * 
 * To establish an ISA interrupt handler, a driver calls isa_intr_establish()
 * with the interrupt number, type, level, function, and function argument of
 * the interrupt it wants to handle.  Isa_intr_establish() returns an opaque
 * handle to an event descriptor if it succeeds, and invokes panic() if it
 * fails.  (XXX It should return NULL, then drivers should handle that, but
 * what should they do?)  Interrupt handlers should return 0 for "interrupt
 * not for me", 1  for "I took care of it", or -1 for "I guess it was mine,
 * but I wasn't expecting it."
 *
 * To remove an interrupt handler, the driver calls isa_intr_disestablish() 
 * with the handle returned by isa_intr_establish() for that handler.
 */

/* ISA interrupt sharing types */

/*
 * Some ISA devices (e.g. on a VLB) can perform 32-bit DMA.  This
 * flag is passed to bus_dmamap_create() to indicate that fact.
 */
#define	ISABUS_DMA_32BIT	BUS_DMA_BUS1

/* ISA Hints */
int		isahint_match(struct isabus_attach_args *, struct cfdata *);
void	isahint_attach(struct isa_softc *);

#endif /* _DEV_ISA_ISAVAR_H_ */
