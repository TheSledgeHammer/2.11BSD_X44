/*	$NetBSD: usb_mem.c,v 1.27.6.1 2005/05/13 17:09:07 riz Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
/*
 * USB DMA memory allocation.
 * We need to allocate a lot of small (many 8 byte, some larger)
 * memory blocks that can be used for DMA.  Using the bus_dma
 * routines directly would incur large overheads in space and time.
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/user.h>

#ifdef DIAGNOSTIC
#include <sys/proc.h>
#endif

#include <machine/bus.h>

#include <sys/extent.h>
#include <vm/include/vm_extern.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>	/* just for usb_dma_t */
#include <dev/usb/usb_mem.h>

#ifdef USB_DEBUG
#define DPRINTF(x)		if (usbdebug) printf x
#define DPRINTFN(n,x)	if (usbdebug>(n)) printf x
extern int usbdebug;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define USB_MEM_SMALL 	64
#define USB_MEM_CHUNKS 	64
#define USB_MEM_BLOCK 	(USB_MEM_SMALL * USB_MEM_CHUNKS)

/* This struct is overlayed on free fragments. */
struct usb_frag_dma {
	usb_dma_block_t 			*block;
	u_int 						offs;
	LIST_ENTRY(usb_frag_dma) 	next;
};

static usbd_status	usb_block_allocmem(bus_dma_tag_t, size_t, size_t, usb_dma_block_t **);
static void			usb_block_real_freemem(usb_dma_block_t *);
static void			usb_block_freemem(usb_dma_block_t *);

static LIST_HEAD(, usb_dma_block) usb_blk_freelist = LIST_HEAD_INITIALIZER(usb_blk_freelist);
static int usb_blk_nfree = 0;
/* XXX should have different free list for different tags */
static LIST_HEAD(, usb_frag_dma) usb_frag_freelist = LIST_HEAD_INITIALIZER(usb_frag_freelist);

static usbd_status
usb_block_allocmem(tag, size, align, dmap)
	bus_dma_tag_t tag;
	size_t size;
	size_t align;
    usb_dma_block_t **dmap;
{
	int error;
        usb_dma_block_t *p;
	int s;

	DPRINTFN(5, ("usb_block_allocmem: size=%d align=%d\n", size, align));

#ifdef DIAGNOSTIC
	if (!curproc) {
		printf("usb_block_allocmem: in interrupt context\n");
		return (USBD_NOMEM);
	}
#endif

	s = splusb();
	/* First check the free list. */
	for (p = LIST_FIRST(&usb_blk_freelist); p; p = LIST_NEXT(p, next)) {
		if (p->tag == tag && p->size >= size && p->align >= align) {
			LIST_REMOVE(p, next);
			splx(s);
			*dmap = p;
			DPRINTFN(6, ("usb_block_allocmem: free list size=%d\n",
				     p->size));
			return (USBD_NORMAL_COMPLETION);
		}
	}
	splx(s);

	DPRINTFN(6, ("usb_block_allocmem: no free\n"));
	p = malloc(sizeof *p, M_USB, M_NOWAIT);
	if (p == 0)
		return (USBD_NOMEM);
	*dmap = p;

	p->tag = tag;
	p->size = size;
	p->align = align;
	error = bus_dmamem_alloc(tag, p->size, align, 0,
				 p->segs, sizeof(p->segs)/sizeof(p->segs[0]), &p->nsegs, BUS_DMA_NOWAIT);
	if (error)
		return (USBD_NOMEM);

	error = bus_dmamem_map(tag, p->segs, p->nsegs, p->size, &p->kaddr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(tag, p->size, 1, p->size, 0, BUS_DMA_NOWAIT, &p->map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(tag, p->map, p->kaddr,p->size, NULL, BUS_DMA_NOWAIT);
	if (error)
		goto destroy;
	return 0;

destroy:
	bus_dmamap_destroy(tag, p->map);
unmap:
	bus_dmamem_unmap(tag, p->kaddr, p->size);
free:
	bus_dmamem_free(tag, p->segs, p->nsegs);
	return (USBD_NOMEM);
}

void
usb_block_real_freemem(p)
        usb_dma_block_t *p;
{
#ifdef DIAGNOSTIC
	if (!curproc) {
		printf("usb_block_real_freemem: in interrupt context\n");
		return;
	}
#endif
	bus_dmamap_unload(p->tag, p->map);
	bus_dmamap_destroy(p->tag, p->map);
	bus_dmamem_unmap(p->tag, p->kaddr, p->size);
	bus_dmamem_free(p->tag, p->segs, p->nsegs);
	free(p, M_USB);
}

/*
 * Do not free the memory unconditionally since we might be called
 * from an interrupt context and that is BAD.
 * XXX when should we really free?
 */
static void
usb_block_freemem(p)
        usb_dma_block_t *p;
{
	int s;

	DPRINTFN(6, ("usb_block_freemem: size=%d\n", p->size));
	s = splusb();
	LIST_INSERT_HEAD(&usb_blk_freelist, p, next);
	usb_blk_nfree++;
	splx(s);
}

usbd_status
usb_allocmem(tag, size, align, p)
	bus_dma_tag_t tag;
	size_t size;
	size_t align;
        usb_dma_t *p;
{
	usbd_status r;
	struct usb_frag_dma *f;
	usb_dma_block_t *b;
	int i;
	int s;

	/* If the request is large then just use a full block. */
	if (size > USB_MEM_SMALL || align > USB_MEM_SMALL) {
		DPRINTFN(1, ("usb_allocmem: large alloc %d\n", (int)size));
		size = (size + USB_MEM_BLOCK - 1) & ~(USB_MEM_BLOCK - 1);
		r = usb_block_allocmem(tag, size, align, &p->block);
		if (r == USBD_NORMAL_COMPLETION) {
			p->block->flags = USB_DMA_FULLBLOCK;
			p->offs = 0;
		}
		return (r);
	}
	
	s = splusb();
	/* Check for free fragments. */
	for (f = LIST_FIRST(&usb_frag_freelist); f; f = LIST_NEXT(f, next))
		if (f->block->tag == tag)
			break;
	if (!f) {
		DPRINTFN(1, ("usb_allocmem: adding fragments\n"));
		r = usb_block_allocmem(tag, USB_MEM_BLOCK, USB_MEM_SMALL, &b);
		if (r != USBD_NORMAL_COMPLETION) {
			splx(s);
			return (r);
		}
		b->flags = 0;
		for (i = 0; i < USB_MEM_BLOCK; i += USB_MEM_SMALL) {
			f = (struct usb_frag_dma *)(b->kaddr + i);
			f->block = b;
			f->offs = i;
			LIST_INSERT_HEAD(&usb_frag_freelist, f, next);
		}
		f = LIST_FIRST(&usb_frag_freelist);
	}
	p->block = f->block;
	p->offs = f->offs;
	p->block->flags &= ~USB_DMA_RESERVE;
	LIST_REMOVE(f, next);
	splx(s);
	DPRINTFN(5, ("usb_allocmem: use frag=%p size=%d\n", f, (int)size));
	return (USBD_NORMAL_COMPLETION);
}

void
usb_freemem(tag, p)
	bus_dma_tag_t tag;
        usb_dma_t *p;
{
	struct usb_frag_dma *f;
	int s;

	if (p->block->flags & USB_DMA_FULLBLOCK) {
		DPRINTFN(1, ("usb_freemem: large free\n"));
		usb_block_freemem(p->block);
		return;
	}
	f = KERNADDR(p, 0);
	f->block = p->block;
	f->offs = p->offs;
	s = splusb();
	LIST_INSERT_HEAD(&usb_frag_freelist, f, next);
	splx(s);
	DPRINTFN(5, ("usb_freemem: frag=%p\n", f));
}

usbd_status
usb_reserve_allocm(rs, dma, size)
	struct usb_dma_reserve *rs;
	usb_dma_t *dma;
	u_int32_t size;
{
	int error;
	u_long start;
	bus_addr_t baddr;

	if (rs->vaddr == 0)
		return USBD_NOMEM;

	dma->block = malloc(sizeof *dma->block, M_USB, M_ZERO | M_NOWAIT);
	if (dma->block == NULL)
		return USBD_NOMEM;

	error = extent_alloc(rs->extent, size, PAGE_SIZE, 0,
	    EX_NOWAIT, &start);

	if (error != 0) {
		printf("%s: usb_reserve_allocm of size %u failed (error %d)\n",
		    ((struct device *)rs->softc)->dv_xname, size, error);
		return USBD_NOMEM;
	}

	baddr = start;
	dma->offs = baddr - rs->paddr;
	dma->block->flags = USB_DMA_RESERVE;
	dma->block->align = PAGE_SIZE;
	dma->block->size = size;
	dma->block->nsegs = 1;
	/* XXX segs appears to be unused */
	dma->block->segs[0] = rs->map->dm_segs[0];
	dma->block->map = rs->map;
	dma->block->kaddr = rs->vaddr;
	dma->block->tag = rs->dtag;

	return USBD_NORMAL_COMPLETION;
}

void
usb_reserve_freem(rs, dma)
	struct usb_dma_reserve *rs;
	usb_dma_t *dma;
{
	int error;

	error = extent_free(rs->extent,
	    (u_long)(rs->paddr + dma->offs), dma->block->size, 0);
	free(dma->block, M_USB);
}

int
usb_setup_reserve(softc, rs, dtag, size)
	void *softc;
	struct usb_dma_reserve *rs;
	bus_dma_tag_t dtag;
	size_t size;
{
	int error, nseg;
	bus_dma_segment_t seg;
	struct device *dv = softc;

	rs->dtag = dtag;
	rs->size = size;
	rs->softc = softc;

	error = bus_dmamem_alloc(dtag, USB_MEM_RESERVE, PAGE_SIZE, 0,
	    &seg, 1, &nseg, BUS_DMA_NOWAIT);
	if (error != 0)
		return error;

	error = bus_dmamem_map(dtag, &seg, nseg, USB_MEM_RESERVE,
	    &rs->vaddr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error != 0)
		goto freeit;

	error = bus_dmamap_create(dtag, USB_MEM_RESERVE, 1,
	    USB_MEM_RESERVE, 0, BUS_DMA_NOWAIT, &rs->map);
	if (error != 0)
		goto unmap;

	error = bus_dmamap_load(dtag, rs->map, rs->vaddr, USB_MEM_RESERVE,
	    NULL, BUS_DMA_NOWAIT);
	if (error != 0)
		goto destroy;

	rs->paddr = rs->map->dm_segs[0].ds_addr;
	rs->extent = extent_create(dv->dv_xname, (u_long)rs->paddr,
	    (u_long)(rs->paddr + USB_MEM_RESERVE),
	    M_USB, 0, 0, 0);
	if (rs->extent == NULL) {
		rs->vaddr = 0;
		return ENOMEM;
	}

	return 0;

 destroy:
	bus_dmamap_destroy(dtag, rs->map);
 unmap:
	bus_dmamem_unmap(dtag, rs->vaddr, size);
 freeit:
	bus_dmamem_free(dtag, &seg, nseg);

	rs->vaddr = 0;

	return error;
}
