/*	$NetBSD: isa_machdep.c,v 1.23.2.1 1997/11/13 08:11:04 mellon Exp $	*/

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

/*-
 * Copyright (c) 1993, 1994, 1996, 1997
 *	Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */

#define ISA_DMA_STATS

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#define _I386_BUS_DMA_PRIVATE

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pic.h>
//#include <machine/pio.h>
#include <machine/cpufunc.h>
//#include <machine/segments.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>
#include <dev/core/isa/isadmavar.h>

#include <machine/isa/isa_machdep.h>

#include <vm/include/vm.h>

extern	vm_offset_t avail_end;

/*
 * ISA can only DMA to 0-16M.
 */
#define	ISA_DMA_BOUNCE_THRESHOLD	0x00ffffff

int		_isa_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t, bus_size_t, int, bus_dmamap_t *);
void	_isa_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int		_isa_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t, struct proc *, int);
int		_isa_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t, struct mbuf *, int);
int		_isa_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t, struct uio *, int);
int		_isa_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int);
void	_isa_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	_isa_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_dmasync_op_t);

int		_isa_bus_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int);
void	_isa_bus_dmamem_free(bus_dma_tag_t, bus_dma_segment_t *, int);
int		_isa_bus_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *, int, size_t, caddr_t *, int);
void	_isa_bus_dmamem_unmap(bus_dma_tag_t, caddr_t, size_t);
//int		_isa_bus_dmamem_mmap(bus_dma_tag_t, bus_dma_segment_t *, int, off_t, int, int);

int		_isa_dma_check_buffer(void *, bus_size_t, int, bus_size_t, struct proc *);
int		_isa_dma_alloc_bouncebuf(bus_dma_tag_t, bus_dmamap_t, bus_size_t, int);
void	_isa_dma_free_bouncebuf(bus_dma_tag_t, bus_dmamap_t);

/*
 * Entry points for ISA DMA.  These are mostly wrappers around
 * the generic functions that understand how to deal with bounce
 * buffers, if necessary.
 */
struct i386_bus_dma_tag isa_bus_dma_tag = {
	NULL,			/* _cookie */
	_isa_bus_dmamap_create,
	_isa_bus_dmamap_destroy,
	_isa_bus_dmamap_load,
	_isa_bus_dmamap_load_mbuf,
	_isa_bus_dmamap_load_uio,
	_isa_bus_dmamap_load_raw,
	_isa_bus_dmamap_unload,
	_isa_bus_dmamap_sync,
	_isa_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dmamem_alloc_range
};

/**********************************************************************
 * bus_dma.h dma interface entry points
 **********************************************************************/

#ifdef ISA_DMA_STATS
#define	STAT_INCR(v)	(v)++
#define	STAT_DECR(v) do { 										\
	if ((v) == 0) { 											\
		printf("%s:%d -- Already 0!\n", __FILE__, __LINE__); 	\
	} else {													\
		(v)--; 													\
	}															\
} while (0)

u_long	isa_dma_stats_loads;
u_long	isa_dma_stats_bounces;
u_long	isa_dma_stats_nbouncebufs;
#else
#define	STAT_INCR(v)
#define	STAT_DECR(v)
#endif

/*
 * Create an ISA DMA map.
 */
int
_isa_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct i386_isa_dma_cookie *cookie;
	bus_dmamap_t map;
	int error, cookieflags;
	void *cookiestore;
	size_t cookiesize;

	/* Call common function to create the basic map. */
	error = _bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp);
	if (error)
		return (error);

	map = *dmamp;
	map->_dm_cookie = NULL;

	cookiesize = sizeof(struct i386_isa_dma_cookie);

	/*
	 * ISA only has 24-bits of address space.  This means
	 * we can't DMA to pages over 16M.  In order to DMA to
	 * arbitrary buffers, we use "bounce buffers" - pages
	 * in memory below the 16M boundary.  On DMA reads,
	 * DMA happens to the bounce buffers, and is copied into
	 * the caller's buffer.  On writes, data is copied into
	 * but bounce buffer, and the DMA happens from those
	 * pages.  To software using the DMA mapping interface,
	 * this looks simply like a data cache.
	 *
	 * If we have more than 16M of RAM in the system, we may
	 * need bounce buffers.  We check and remember that here.
	 *
	 * There are exceptions, however.  VLB devices can do
	 * 32-bit DMA, and indicate that here.
	 *
	 * ...or, there is an opposite case.  The most segments
	 * a transfer will require is (maxxfer / NBPG) + 1.  If
	 * the caller can't handle that many segments (e.g. the
	 * ISA DMA controller), we may have to bounce it as well.
	 */
	cookieflags = 0;
	if ((avail_end > ISA_DMA_BOUNCE_THRESHOLD &&
	    (flags & ISABUS_DMA_32BIT) == 0) ||
	    ((map->_dm_size / PAGE_SIZE) + 1) > map->_dm_segcnt) {
		cookieflags |= ID_MIGHT_NEED_BOUNCE;
		cookiesize += (sizeof(bus_dma_segment_t) * map->_dm_segcnt);
	}

	/*
	 * Allocate our cookie.
	 */
	if ((cookiestore = malloc(cookiesize, M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL) {
		error = ENOMEM;
		goto out;
	}
	bzero(cookiestore, cookiesize);
	cookie = (struct i386_isa_dma_cookie *)cookiestore;
	cookie->id_flags = cookieflags;
	map->_dm_cookie = cookie;

	if (cookieflags & ID_MIGHT_NEED_BOUNCE) {
		/*
		 * Allocate the bounce pages now if the caller
		 * wishes us to do so.
		 */
		if ((flags & BUS_DMA_ALLOCNOW) == 0)
			goto out;

		error = _isa_dma_alloc_bouncebuf(t, map, size, flags);
	}

 out:
	if (error) {
		if (map->_dm_cookie != NULL)
			free(map->_dm_cookie, M_DEVBUF);
		_bus_dmamap_destroy(t, map);
	}
	return (error);
}

/*
 * Destroy an ISA DMA map.
 */
void
_isa_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	struct i386_isa_dma_cookie *cookie = map->_dm_cookie;

	/*
	 * Free any bounce pages this map might hold.
	 */
	if (cookie->id_flags & ID_HAS_BOUNCE)
		_isa_dma_free_bouncebuf(t, map);

	free(cookie, M_DEVBUF);
	_bus_dmamap_destroy(t, map);
}

/*
 * Load an ISA DMA map with a linear buffer.
 */
int
_isa_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	struct i386_isa_dma_cookie *cookie = map->_dm_cookie;
	int error;

	STAT_INCR(isa_dma_stats_loads);

	/*
	 * Check to see if we might need to bounce the transfer.
	 */
	if (cookie->id_flags & ID_MIGHT_NEED_BOUNCE) {
		/*
		 * Check if all pages are below the bounce
		 * threshold.  If they are, don't bother bouncing.
		 */
		if (_isa_dma_check_buffer(buf, buflen,
		    map->_dm_segcnt, map->_dm_boundary, p) == 0)
			return (_bus_dmamap_load(t, map, buf, buflen, p, flags));

		STAT_INCR(isa_dma_stats_bounces);

		/*
		 * Allocate bounce pages, if necessary.
		 */
		if ((cookie->id_flags & ID_HAS_BOUNCE) == 0) {
			error = _isa_dma_alloc_bouncebuf(t, map, buflen,
			    flags);
			if (error)
				return (error);
		}

		/*
		 * Cache a pointer to the caller's buffer and
		 * load the DMA map with the bounce buffer.
		 */
		cookie->id_origbuf = buf;
		cookie->id_origbuflen = buflen;
		error = _bus_dmamap_load(t, map, cookie->id_bouncebuf, buflen, p, flags);

		if (error) {
			/*
			 * Free the bounce pages, unless our resources
			 * are reserved for our exclusive use.
			 */
			if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
				_isa_dma_free_bouncebuf(t, map);
		}

		/* ...so _isa_bus_dmamap_sync() knows we're bouncing */
		cookie->id_flags |= ID_IS_BOUNCING;
	} else {
		/*
		 * Just use the generic load function.
		 */
		error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	}

	return (error);
}

/*
 * Like _isa_bus_dmamap_load(), but for mbufs.
 */
int
_isa_bus_dmamap_load_mbuf(t, map, m0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
{

	panic("_isa_bus_dmamap_load_mbuf: not implemented");
	struct i386_isa_dma_cookie *cookie = map->_dm_cookie;
	int error;

	/*
	 * Make sure on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
		if ((m0->m_flags & M_PKTHDR) == 0)
			panic("_isa_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_cont->mc_pkthdr.len > map->_dm_size)
		return (EINVAL);

	/*
	 * Try to load the map the normal way.  If this errors out,
	 * and we can bounce, we will.
	 */
	error = _bus_dmamap_load_mbuf(t, map, m0, flags);
	if (error == 0
			|| (error != 0 && (cookie->id_flags & ID_MIGHT_NEED_BOUNCE) == 0))
		return (error);

	/*
	 * First attempt failed; bounce it.
	 */

	STAT_INCR(isa_dma_stats_bounces);

	/*
	 * Allocate bounce pages, if necessary.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) == 0) {
		error = _isa_dma_alloc_bouncebuf(t, map, m0->m_cont->mc_pkthdr.len, flags);
		if (error)
			return (error);
	}

	/*
	 * Cache a pointer to the caller's buffer and load the DMA map
	 * with the bounce buffer.
	 */
	cookie->id_origbuf = m0;
	cookie->id_origbuflen = m0->m_pkthdr.len; /* not really used */
	cookie->id_buftype = ID_BUFTYPE_MBUF;
	error = _bus_dmamap_load(t, map, cookie->id_bouncebuf, m0->m_cont->mc_pkthdr.len, NULL, flags);
	if (error) {
		/*
		 * Free the bounce pages, unless our resources
		 * are reserved for our exclusive use.
		 */
		if ((map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
			_isa_dma_free_bouncebuf(t, map);
		return (error);
	}

	/* ...so _isa_bus_dmamap_sync() knows we're bouncing */
	cookie->id_flags |= ID_IS_BOUNCING;
	return (0);
}

/*
 * Like _isa_bus_dmamap_load(), but for uios.
 */
int
_isa_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{

	panic("_isa_bus_dmamap_load_uio: not implemented");
}

/*
 * Like _isa_bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_isa_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{

	panic("_isa_bus_dmamap_load_raw: not implemented");
}

/*
 * Unload an ISA DMA map.
 */
void
_isa_bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	struct i386_isa_dma_cookie *cookie = map->_dm_cookie;

	/*
	 * If we have bounce pages, free them, unless they're
	 * reserved for our exclusive use.
	 */
	if ((cookie->id_flags & ID_HAS_BOUNCE) &&
	    (map->_dm_flags & BUS_DMA_ALLOCNOW) == 0)
		_isa_dma_free_bouncebuf(t, map);

	cookie->id_flags &= ~ID_IS_BOUNCING;

	/*
	 * Do the generic bits of the unload.
	 */
	_bus_dmamap_unload(t, map);
}

/*
 * Synchronize an ISA DMA map.
 */
void
_isa_bus_dmamap_sync(t, map, op)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dmasync_op_t op;
{
	struct i386_isa_dma_cookie *cookie = map->_dm_cookie;

	switch (op) {
	case BUS_DMASYNC_PREREAD:
		/*
		 * Nothing to do for pre-read.
		 */
		break;

	case BUS_DMASYNC_PREWRITE:
		/*
		 * If we're bouncing this transfer, copy the
		 * caller's buffer to the bounce buffer.
		 */
		if (cookie->id_flags & ID_IS_BOUNCING)
			bcopy(cookie->id_origbuf, cookie->id_bouncebuf,
			    cookie->id_origbuflen);
		break;

	case BUS_DMASYNC_POSTREAD:
		/*
		 * If we're bouncing this transfer, copy the
		 * bounce buffer to the caller's buffer.
		 */
		if (cookie->id_flags & ID_IS_BOUNCING)
			bcopy(cookie->id_bouncebuf, cookie->id_origbuf,
			    cookie->id_origbuflen);
		break;

	case BUS_DMASYNC_POSTWRITE:
		/*
		 * Nothing to do for post-write.
		 */
		break;
	}

#if 0
	/* This is a noop anyhow, so why bother calling it? */
	bus_dmamap_sync(t, map, op);
#endif
}

/*
 * Allocate memory safe for ISA DMA.
 */
int
_isa_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	vm_offset_t high;

	if (avail_end > ISA_DMA_BOUNCE_THRESHOLD)
		high = trunc_page(ISA_DMA_BOUNCE_THRESHOLD);
	else
		high = trunc_page(avail_end);

	return (_bus_dmamem_alloc_range(t, size, alignment, boundary, segs, nsegs, rsegs, flags, 0, high));
}

/**********************************************************************
 * ISA DMA utility functions
 **********************************************************************/

/*
 * Return 0 if all pages in the passed buffer lie within the DMA'able
 * range RAM.
 */
int
_isa_dma_check_buffer(buf, buflen, segcnt, boundary, p)
	void *buf;
	bus_size_t buflen;
	int segcnt;
	bus_size_t boundary;
	struct proc *p;
{
	vm_offset_t vaddr = (vm_offset_t)buf;
	vm_offset_t pa, lastpa, endva;
	u_long pagemask = ~(boundary - 1);
	pmap_t pmap;
	int nsegs;

	endva = round_page(vaddr + buflen);

	nsegs = 1;
	lastpa = 0;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (; vaddr < endva; vaddr += NBPG) {
		/*
		 * Get physical address for this segment.
		 */
		pa = pmap_extract(pmap, (vm_offset_t)vaddr);
		pa = trunc_page(pa);

		/*
		 * Is it below the DMA'able threshold?
		 */
		if (pa > ISA_DMA_BOUNCE_THRESHOLD)
			return (EINVAL);

		if (lastpa) {
			/*
			 * Check excessive segment count.
			 */
			if (lastpa + NBPG != pa) {
				if (++nsegs > segcnt)
					return (EFBIG);
			}

			/*
			 * Check boundary restriction.
			 */
			if (boundary) {
				if ((lastpa ^ pa) & pagemask)
					return (EINVAL);
			}
		}
		lastpa = pa;
	}

	return (0);
}

int
_isa_dma_alloc_bouncebuf(t, map, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_size_t size;
	int flags;
{
	struct i386_isa_dma_cookie *cookie = map->_dm_cookie;
	int error = 0;

	cookie->id_bouncebuflen = round_page(size);
	error = _isa_bus_dmamem_alloc(t, cookie->id_bouncebuflen,
	    NBPG, map->_dm_boundary, cookie->id_bouncesegs,
	    map->_dm_segcnt, &cookie->id_nbouncesegs, flags);
	if (error)
		goto out;
	error = _isa_bus_dmamem_map(t, cookie->id_bouncesegs,
	    cookie->id_nbouncesegs, cookie->id_bouncebuflen,
	    (caddr_t *)&cookie->id_bouncebuf, flags);

 out:
	if (error) {
		_isa_bus_dmamem_free(t, cookie->id_bouncesegs,
		    cookie->id_nbouncesegs);
		cookie->id_bouncebuflen = 0;
		cookie->id_nbouncesegs = 0;
	} else {
		cookie->id_flags |= ID_HAS_BOUNCE;
		STAT_INCR(isa_dma_stats_nbouncebufs);
	}

	return (error);
}

void
_isa_dma_free_bouncebuf(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	struct i386_isa_dma_cookie *cookie = map->_dm_cookie;

	STAT_DECR(isa_dma_stats_nbouncebufs);

	_isa_bus_dmamem_unmap(t, cookie->id_bouncebuf, cookie->id_bouncebuflen);
	_isa_bus_dmamem_free(t, cookie->id_bouncesegs, cookie->id_nbouncesegs);
	cookie->id_bouncebuflen = 0;
	cookie->id_nbouncesegs = 0;
	cookie->id_flags &= ~ID_HAS_BOUNCE;
}
