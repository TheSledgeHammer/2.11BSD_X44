/*	$NetBSD: bus.h,v 1.12 1997/10/01 08:25:15 fvdl Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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

#ifndef _I386_BUS_DMA_H_
#define _I386_BUS_DMA_H_

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
typedef struct i386_bus_dma_segment {
	bus_addr_t			ds_addr;			/* DMA address */
	bus_size_t			ds_len;				/* length of transfer */
} bus_dma_segment_t;

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct i386_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_size_t			_dm_size;			/* largest DMA transfer mappable */
	int					_dm_segcnt;			/* number of segs this map can map */
	bus_size_t			_dm_maxmaxsegsz; 	/* fixed largest possible segment */
	bus_size_t			_dm_boundary;		/* don't cross this */
	bus_addr_t			_dm_bounce_thresh; 	/* bounce threshold; see tag */
	int					_dm_flags;			/* misc. flags */

	void				*_dm_cookie;		/* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t			dm_maxsegsz;		/* largest possible segment */
	bus_size_t			dm_mapsize;			/* size of the mapping */
	int					dm_nsegs;			/* # valid segments in mapping */
	bus_dma_segment_t 	dm_segs[1];			/* segments; variable length */
};

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct i386_bus_dma_tag {
	void	 *_cookie;		/* cookie used in the guts */

	bus_addr_t _bounce_thresh;
//	bus_addr_t _bounce_alloc_lo;
//	bus_addr_t _bounce_alloc_hi;
//	int		(*_may_bounce)(bus_dma_tag_t, bus_dmamap_t, int, int *);

	/*
	 * DMA mapping methods.
	 */
	int		(*_dmamap_create) (bus_dma_tag_t, bus_size_t, int, bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*_dmamap_destroy) (bus_dma_tag_t, bus_dmamap_t);
	int		(*_dmamap_load) (bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t, struct proc *, int);
	int		(*_dmamap_load_mbuf) (bus_dma_tag_t, bus_dmamap_t, struct mbuf *, int);
	int		(*_dmamap_load_uio) (bus_dma_tag_t, bus_dmamap_t, struct uio *, int);
	int		(*_dmamap_load_raw) (bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int);
	void	(*_dmamap_unload) (bus_dma_tag_t, bus_dmamap_t);
	void	(*_dmamap_sync) (bus_dma_tag_t, bus_dmamap_t, bus_dmasync_op_t);

	/*
	 * DMA memory utility functions.
	 */
	int		(*_dmamem_alloc) (bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*_dmamem_free) (bus_dma_tag_t, bus_dma_segment_t *, int);
	int		(*_dmamem_map) (bus_dma_tag_t, bus_dma_segment_t *, int, size_t, caddr_t *, int);
	void	(*_dmamem_unmap) (bus_dma_tag_t, caddr_t, size_t);
	int		(*_dmamem_mmap) (bus_dma_tag_t, bus_dma_segment_t *, int, int, int, int);
};
typedef struct i386_bus_dma_tag		*bus_dma_tag_t;

#define	bus_dmamap_create(t, s, n, m, b, f, p)		\
	(*(t)->_dmamap_create)((t), (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)					\
	(*(t)->_dmamap_destroy)((t), (p))
#define	bus_dmamap_load(t, m, b, s, p, f)			\
	(*(t)->_dmamap_load)((t), (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)			\
	(*(t)->_dmamap_load_mbuf)((t), (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)				\
	(*(t)->_dmamap_load_uio)((t), (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)		\
	(*(t)->_dmamap_load_raw)((t), (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)						\
	(*(t)->_dmamap_unload)((t), (p))
#define	bus_dmamap_sync(t, p, o)					\
	(void)((t)->_dmamap_sync ?						\
	    (*(t)->_dmamap_sync)((t), (p), (o)) : (void)0)

#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)	\
	(*(t)->_dmamem_alloc)((t), (s), (a), (b), (sg), (n), (r), (f))
#define	bus_dmamem_free(t, sg, n)					\
	(*(t)->_dmamem_free)((t), (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)			\
	(*(t)->_dmamem_map)((t), (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)					\
	(*(t)->_dmamem_unmap)((t), (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)			\
	(*(t)->_dmamem_mmap)((t), (sg), (n), (o), (p), (f))


int	i386_memio_map (bus_space_tag_t t, bus_addr_t addr, bus_size_t size, int flags, bus_space_handle_t *bshp);
/* like map, but without extent map checking/allocation */
int	_i386_memio_map (bus_space_tag_t t, bus_addr_t addr, bus_size_t size, int flags, bus_space_handle_t *bshp);
void i386_memio_unmap (bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size);
int	i386_memio_subregion (bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp);
int	i386_memio_alloc (bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend, bus_size_t size, bus_size_t align, bus_size_t boundary, int flags, bus_addr_t *addrp, bus_space_handle_t *bshp);
void i386_memio_free (bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size);
#endif /* _I386_BUS_DMA_H_ */
