/*	$NetBSD: bus.h,v 1.13 2020/03/08 02:42:00 thorpej Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#ifndef _SYS_BUS_H_
#define	_SYS_BUS_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <devel/arch/i386/include/bus.h>

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
struct uio;

/* bus_space(9) */

/* Map types. */
#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define	BUS_SPACE_MAP_PREFETCHABLE	0x04

/* Bus read/write barrier methods. */
#define	BUS_SPACE_BARRIER_READ		0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE		0x02		/* force write barrier */

int			bus_space_map(bus_space_tag_t, bus_addr_t, bus_size_t, int, bus_space_handle_t *);
void		bus_space_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
int			bus_space_subregion(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_size_t, bus_space_handle_t *);
int			bus_space_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void		bus_space_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);
caddr_t		bus_space_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
void		*bus_space_vaddr(bus_space_tag_t, bus_space_handle_t);

//void		bus_space_barrier(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_size_t, int);

u_int8_t	bus_space_read_1(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int16_t 	bus_space_read_2(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int32_t 	bus_space_read_4(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int64_t 	bus_space_read_8(bus_space_tag_t, bus_space_handle_t, bus_size_t);

u_int8_t	bus_space_read_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int16_t 	bus_space_read_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int32_t 	bus_space_read_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int64_t 	bus_space_read_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t);

void		bus_space_read_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, uint8_t *, size_t);
void		bus_space_read_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, uint16_t *, size_t);
void		bus_space_read_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *, size_t);
void		bus_space_read_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *, size_t);

void		bus_space_read_multi_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t *, size_t);
void		bus_space_read_multi_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *, size_t);
void		bus_space_read_multi_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *, size_t);
void		bus_space_read_multi_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *, size_t);

void		bus_space_read_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_read_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_read_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_read_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_read_region_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_read_region_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_read_region_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_read_region_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_write_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t);
void		bus_space_write_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t);
void		bus_space_write_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t);
void		bus_space_write_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t);

void		bus_space_write_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t);
void		bus_space_write_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t);
void		bus_space_write_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t);
void		bus_space_write_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t);

void		bus_space_write_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_write_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_write_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_write_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_write_multi_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_write_multi_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_write_multi_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_write_multi_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_write_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int8_t *, size_t);
void		bus_space_write_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int16_t *, size_t);
void		bus_space_write_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int32_t *, size_t);
void		bus_space_write_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, const u_int64_t *, size_t);

void		bus_space_set_multi_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t, size_t);
void		bus_space_set_multi_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t, size_t);
void		bus_space_set_multi_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t, size_t);
void		bus_space_set_multi_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t, size_t);

void		bus_space_set_multi_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t, size_t);
void		bus_space_set_multi_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t, size_t);
void		bus_space_set_multi_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t, size_t);
void		bus_space_set_multi_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t, size_t);

void		bus_space_set_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t, size_t);
void		bus_space_set_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t, size_t);
void		bus_space_set_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t, size_t);
void		bus_space_set_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t, size_t);

void		bus_space_set_region_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t, size_t);
void		bus_space_set_region_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t, size_t);
void		bus_space_set_region_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t, size_t);
void		bus_space_set_region_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t, size_t);

void		bus_space_copy_region_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);

void		bus_space_copy_region_stream_1(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_stream_2(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_stream_4(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void		bus_space_copy_region_stream_8(bus_space_tag_t, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, size_t);

#ifdef __HAVE_NO_BUS_DMA
/* bus_dma(9) */
typedef void *bus_dma_tag_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
typedef struct bus_dma_segment {
	bus_addr_t 			ds_addr;
	bus_size_t 			ds_len;
} bus_dma_segment_t;

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
typedef struct bus_dmamap {
	bus_size_t 			dm_maxsegsz;
	bus_size_t 			dm_mapsize;
	int					dm_nsegs;		/* # valid segments in mapping */
	bus_dma_segment_t 	dm_segs[1];		/* segments; variable length */
} *bus_dmamap_t;

#else
#include <machine/bus_dma.h>
#endif /* __HAVE_NO_BUS_DMA */

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
typedef enum {
	BUS_DMASYNC_PREREAD 	= 	0x01,	/* pre-read synchronization */
	BUS_DMASYNC_POSTREAD	= 	0x02,	/* post-read synchronization */
	BUS_DMASYNC_PREWRITE	= 	0x04,	/* pre-write synchronization */
	BUS_DMASYNC_POSTWRITE	= 	0x08,	/* post-write synchronization */
} bus_dmasync_op_t;

int		bus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t, bus_size_t, int, bus_dmamap_t *);
void	bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int		bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t, struct proc *, int);
int		bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t, struct mbuf *, int);
int		bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t, struct uio *, int);
int		bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int);
void	bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_dmasync_op_t);

int		bus_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int);
void	bus_dmamem_free(bus_dma_tag_t, bus_dma_segment_t *, int);
int		bus_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *, int, size_t, void **, int);
void	bus_dmamem_unmap(bus_dma_tag_t, void *, size_t);
int		bus_dmamem_mmap(bus_dma_tag_t, bus_dma_segment_t *, int, off_t, int, int);
int		bus_dmamem_alloc_range(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int, int, vm_offset_t, vm_offset_t);

#endif	/* _SYS_BUS_H_ */
