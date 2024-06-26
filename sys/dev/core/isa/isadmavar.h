/*	$NetBSD: isadmavar.h,v 1.19 2003/05/09 23:51:29 fvdl Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
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

#ifndef _DEV_ISA_ISADMAVAR_H_
#define	_DEV_ISA_ISADMAVAR_H_

#define MAX_ISADMA			65536

#define	DMAMODE_WRITE		0x00
#define	DMAMODE_READ		0x01
#define	DMAMODE_SINGLE		0x00
#define	DMAMODE_DEMAND		0x02
#define	DMAMODE_LOOP		0x04
#define	DMAMODE_LOOPDEMAND	(DMAMODE_LOOP | DMAMODE_DEMAND)

/*
 * ISA DMA state.  This structure is provided by the ISA chipset
 * DMA entry points to the generic back-end functions that actually
 * from the controller.
 */
struct isa_dma_state {
	struct device 		*ids_dev;			/* associated device (for dv_xname) */
	bus_space_tag_t 	ids_bst;			/* bus space tag for DMA controller */
	bus_space_handle_t 	ids_dma1h;			/* handle for DMA controller #1 */
	bus_space_handle_t 	ids_dma2h;			/* handle for DMA controller #2 */
	bus_space_handle_t 	ids_dmapgh;			/* handle for DMA page registers */
	bus_dma_tag_t 		ids_dmat;			/* DMA tag for DMA controller */
	bus_dmamap_t 		ids_dmamaps[8];		/* DMA maps for each channel */
	bus_size_t 			ids_dmalength[8];	/* size of DMA transfer per channel */
	bus_size_t 			ids_maxsize[8];		/* max size per channel */
	int					ids_drqmap;			/* available DRQs (bitmap) */
	int					ids_dmareads;		/* state for isa_dmadone() (bitmap) */
	int					ids_dmafinished;	/* DMA completion state (bitmap) */
	int					ids_masked;			/* masked channels (bitmap) */
	int					ids_frozen;			/* `frozen' count */
	int					ids_initialized;	/* only initialize once... */
};

#define	ISA_DMA_DRQ_ISFREE(state, drq)					\
	(((state)->ids_drqmap & (1 << (drq))) == 0)

#define	ISA_DMA_DRQ_ALLOC(state, drq)					\
	(state)->ids_drqmap |= (1 << (drq))

#define	ISA_DMA_DRQ_FREE(state, drq)					\
	(state)->ids_drqmap &= ~(1 << (drq))

#define	ISA_DMA_MASK_SET(state, drq)					\
	(state)->ids_masked |= (1 << (drq))

#define	ISA_DMA_MASK_CLR(state, drq)					\
	(state)->ids_masked &= ~(1 << (drq))

/*
 * Memory list used by _isa_malloc().
 */
struct isa_mem {
	struct isa_dma_state	*ids;
	isa_chipset_tag_t		ic;
	int 					chan;
	bus_size_t 				size;
	bus_addr_t 				addr;
	caddr_t 				kva;
	SIMPLEQ_ENTRY(isa_mem)	next;
};

#ifdef _KERNEL
struct proc;

void	   	isa_dmainit(isa_chipset_tag_t, bus_space_tag_t, bus_dma_tag_t, struct device *);
int	   	isa_dmacascade(isa_chipset_tag_t, int);

bus_size_t	isa_dmamaxsize(isa_chipset_tag_t, int);

int	  		isa_dmamap_create(isa_chipset_tag_t, int, bus_size_t, int);
void	   	isa_dmamap_destroy(isa_chipset_tag_t, int);

int	   		isa_dmastart(isa_chipset_tag_t, int, void *, bus_size_t, struct proc *, int, int);
void	   	isa_dmaabort(isa_chipset_tag_t, int);
bus_size_t 	isa_dmacount(isa_chipset_tag_t, int);
int	   		isa_dmafinished(isa_chipset_tag_t, int);
void		isa_dmadone(isa_chipset_tag_t, int);

void	   	isa_dmafreeze(isa_chipset_tag_t);
void	   	isa_dmathaw(isa_chipset_tag_t);

int	   		isa_dmamem_alloc(isa_chipset_tag_t, int, bus_size_t, bus_addr_t *, int);
void		isa_dmamem_free(isa_chipset_tag_t, int, bus_addr_t, bus_size_t);
int	   		isa_dmamem_map(isa_chipset_tag_t, int, bus_addr_t, bus_size_t, caddr_t *, int);
void		isa_dmamem_unmap(isa_chipset_tag_t, int, caddr_t, size_t);
int	   		isa_dmamem_mmap(isa_chipset_tag_t, int, bus_addr_t, bus_size_t, int, int, int);

int	   		isa_drq_alloc(isa_chipset_tag_t, int);
int	   		isa_drq_free(isa_chipset_tag_t, int);
int	   		isa_drq_isfree(isa_chipset_tag_t, int);

void    	*isa_malloc(isa_chipset_tag_t, int, size_t, int, int);
void		isa_free(void *, int);
int	   	isa_mappage(void *, int, int);

#endif /* _KERNEL */
#endif /* _DEV_ISA_ISADMAVAR_H_ */
