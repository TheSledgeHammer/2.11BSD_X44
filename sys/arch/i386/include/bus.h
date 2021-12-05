/*	$NetBSD: bus.h,v 1.38 2001/11/10 22:21:00 perry Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2001 The NetBSD Foundation, Inc.
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

#ifndef _I386_BUS_H_
#define _I386_BUS_H_

#include <sys/bus.h>

#include <machine/pio.h>

/*
 * Bus address and size types
 */
typedef u_long 		bus_addr_t;
typedef u_long   	bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef	int 		bus_space_tag_t;
typedef	u_long		bus_space_handle_t;

/*
 * Forwards needed by prototypes below.
 */
struct mbuf;
struct proc;
struct uio;

#ifdef BUS_SPACE_DEBUG
#include <sys/systm.h> /* for printf() prototype */
/*
 * Macros for sanity-checking the aligned-ness of pointers passed to
 * bus space ops.  These are not strictly necessary on the x86, but
 * could lead to performance improvements, and help catch problems
 * with drivers that would creep up on other architectures.
 */
#define	__BUS_SPACE_ALIGNED_ADDRESS(p, t)							\
	((((u_long)(p)) & (sizeof(t)-1)) == 0)

#define	__BUS_SPACE_ADDRESS_SANITY(p, t, d)							\
({																	\
	if (__BUS_SPACE_ALIGNED_ADDRESS((p), t) == 0) {					\
		printf("%s 0x%lx not aligned to %d bytes %s:%d\n",			\
		    d, (u_long)(p), sizeof(t), __FILE__, __LINE__);			\
	}																\
	(void) 0;														\
})

#define BUS_SPACE_ALIGNED_POINTER(p, t) __BUS_SPACE_ALIGNED_ADDRESS(p, t)
#else
#define	__BUS_SPACE_ADDRESS_SANITY(p,t,d)		(void) 0
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)
#endif /* BUS_SPACE_DEBUG */

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * Note: the x86 does not currently require barriers, but we must
 * provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)							\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))

/*
 * Values for the i386 bus space tag, not to be used directly by MI code.
 */
#define	I386_BUS_SPACE_IO				0	/* space is i/o space */
#define I386_BUS_SPACE_MEM				1	/* space is mem space */

#define __BUS_SPACE_HAS_STREAM_METHODS 	1

void 	i386_bus_space_init	(void);
void 	i386_bus_space_mallocok	(void);
void	i386_bus_space_check (vm_offset_t, int, int);

int		i386_memio_map (bus_space_tag_t, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
/* like map, but without extent map checking/allocation */
int		_i386_memio_map (bus_space_tag_t, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
void 	i386_memio_unmap (bus_space_tag_t, bus_space_handle_t, bus_size_t);
int		i386_memio_subregion (bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_size_t, bus_space_handle_t *);
int		i386_memio_alloc (bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void 	i386_memio_free (bus_space_tag_t, bus_space_handle_t, bus_size_t);

/* Bus DMA */
typedef struct i386_bus_dma_segment		*bus_dma_tag_t;
typedef struct i386_bus_dmamap			*bus_dmamap_t;

/* Flags used in various bus DMA methods. */
#define	BUS_DMA_WAITOK			0x000	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT			0x001	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW		0x002	/* perform resource allocation now */
#define	BUS_DMA_COHERENT		0x004	/* hint: map memory DMA coherent */
#define	BUS_DMA_STREAMING		0x008	/* hint: sequential, unidirectional */
#define	BUS_DMA_BUS1			0x010	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2			0x020
#define	BUS_DMA_BUS3			0x040
#define	BUS_DMA_BUS4			0x080
#define	BUS_DMA_READ			0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE			0x200	/* mapping is memory -> device only */
#define	BUS_DMA_NOCACHE			0x400	/* hint: map non-cached memory */
#define	BUS_DMA_PREFETCHABLE	0x800	/* hint: map non-cached but allow things like write combining */

/*
 *	bus_dmasync_op_t
 *	Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD 	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct i386_bus_dma_segment {
	bus_addr_t			ds_addr;		/* DMA address */
	bus_size_t			ds_len;			/* length of transfer */
};
typedef struct i386_bus_dma_segment		bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct i386_bus_dma_tag {
//	bus_addr_t _bounce_thresh;
	void	*_cookie;		/* cookie used in the guts */

	/*
	 * DMA mapping methods.
	 */
	int		(*_dmamap_create)(bus_dma_tag_t, bus_size_t, int, bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*_dmamap_destroy)(bus_dma_tag_t, bus_dmamap_t);
	int		(*_dmamap_load)(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t, struct proc *, int);
	int		(*_dmamap_load_mbuf)(bus_dma_tag_t, bus_dmamap_t, struct mbuf *, int);
	int		(*_dmamap_load_uio)(bus_dma_tag_t, bus_dmamap_t, struct uio *, int);
	int		(*_dmamap_load_raw)(bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int);
	void	(*_dmamap_unload)(bus_dma_tag_t, bus_dmamap_t);
	void	(*_dmamap_sync)(bus_dma_tag_t, bus_dmamap_t, bus_dmasync_op_t);

	/*
	 * DMA memory utility functions.
	 */
	int		(*_dmamem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*_dmamem_free)(bus_dma_tag_t, bus_dma_segment_t *, int);
	int		(*_dmamem_map)(bus_dma_tag_t, bus_dma_segment_t *, int, size_t, caddr_t *, int);
	void	(*_dmamem_unmap)(bus_dma_tag_t, caddr_t, size_t);
	int		(*_dmamem_mmap)(bus_dma_tag_t, bus_dma_segment_t *, int, int, int, int);
	int		(*_dmamem_alloc_range)(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int, vm_offset_t, vm_offset_t);
};

/* macros */
#define	bus_dmamap_create(t, s, n, m, b, f, p)						\
	(*(t)->_dmamap_create)((t), (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)									\
	(*(t)->_dmamap_destroy)((t), (p))
#define	bus_dmamap_load(t, m, b, s, p, f)							\
	(*(t)->_dmamap_load)((t), (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)							\
	(*(t)->_dmamap_load_mbuf)((t), (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)								\
	(*(t)->_dmamap_load_uio)((t), (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)						\
	(*(t)->_dmamap_load_raw)((t), (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)										\
	(*(t)->_dmamap_unload)((t), (p))
#define	bus_dmamap_sync(t, p, o, l, ops)							\
	(void)((t)->_dmamap_sync ?										\
	    (*(t)->_dmamap_sync)((t), (p), (o), (l), (ops)) : (void)0)

#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)					\
	(*(t)->_dmamem_alloc)((t), (s), (a), (b), (sg), (n), (r), (f))
#define	bus_dmamem_free(t, sg, n)									\
	(*(t)->_dmamem_free)((t), (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)							\
	(*(t)->_dmamem_map)((t), (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)									\
	(*(t)->_dmamem_unmap)((t), (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)							\
	(*(t)->_dmamem_mmap)((t), (sg), (n), (o), (p), (f))

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
	bus_size_t			_dm_maxsegsz; 		/* fixed largest possible segment */
	bus_size_t			_dm_boundary;		/* don't cross this */
	bus_addr_t			_dm_bounce_thresh; 	/* bounce threshold; see tag */
	int					_dm_flags;			/* misc. flags */

	void				*_dm_cookie;		/* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t			dm_mapsize;			/* size of the mapping */
	int					dm_nsegs;			/* # valid segments in mapping */
	bus_dma_segment_t 	dm_segs[1];			/* segments; variable length */
};

#ifdef _I386_BUS_DMA_PRIVATE
/* prototypes */
int		_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t, bus_size_t, int, bus_dmamap_t *);
void	_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int		_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t, struct proc *, int);
int		_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t, struct mbuf *, int);
int		_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t, struct uio *, int);
int		_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int);
void	_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t, bus_size_t, int);

int		_bus_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int);
void	_bus_dmamem_free(bus_dma_tag_t, bus_dma_segment_t *, int);
int		_bus_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *, int, size_t, caddr_t *, int);
void	_bus_dmamem_unmap(bus_dma_tag_t, caddr_t, size_t);
int		_bus_dmamem_mmap(bus_dma_tag_t, bus_dma_segment_t *, int, off_t, int, int);
int		_bus_dmamem_alloc_range(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int, vm_offset_t, vm_offset_t);

#endif /* _I386_BUS_DMA_PRIVATE */
#endif /* SYS_DEVEL_ARCH_I386_INCLUDE_BUS_H_ */
