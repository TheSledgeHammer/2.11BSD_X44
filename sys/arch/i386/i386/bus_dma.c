/*	$NetBSD: machdep.c,v 1.262.2.15 1998/11/03 18:52:02 cgd Exp $	*/

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
 * Copyright (c) 1993, 1994, 1995, 1996, 1997
 *	 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1992 Terrence R. Lambert.
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/extent.h>
#include <sys/proc.h>
#include <sys/bus.h>

#include <vm/include/vm_extern.h>

#include <dev/core/isa/isareg.h>

#include <machine/bus_dma.h>
#include <machine/bus_space.h>

#include <i386/isa/isa_machdep.h>

/*
 * Extent maps to manage I/O and ISA memory hole space.  Allocate
 * storage for 8 regions in each, initially.  Later, ioport_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors.
 *
 * N.B. At least two regions are _always_ allocated from the iomem
 * extent map; (0 -> ISA hole) and (end of ISA hole -> end of RAM).
 *
 * The extent maps are not static!  Machine-dependent ISA and EISA
 * routines need access to them for bus address space allocation.
 */
static	long ioport_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
static	long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct	extent *ioport_ex;
struct	extent *iomem_ex;
static	int ioport_malloc_safe;

int	i386_mem_add_mapping (bus_addr_t, bus_size_t, int, bus_space_handle_t *);

void
i386_bus_space_init(void)
{
	/*
	 * Initialize the I/O port and I/O mem extent maps.
	 * Note: we don't have to check the return value since
	 * creation of a fixed extent map will never fail (since
	 * descriptor storage has already been allocated).
	 *
	 * N.B. The iomem extent manages _all_ physical addresses
	 * on the machine.  When the amount of RAM is found, the two
	 * extents of RAM are allocated from the map (0 -> ISA hole
	 * and end of ISA hole -> end of RAM).
	 */

	ioport_ex = extent_create("ioport", 0x0, 0xffff, M_DEVBUF,
		    (caddr_t)ioport_ex_storage, sizeof(ioport_ex_storage),
		    EX_NOCOALESCE|EX_NOWAIT);
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff, M_DEVBUF,
		    (caddr_t)iomem_ex_storage, sizeof(iomem_ex_storage),
		    EX_NOCOALESCE|EX_NOWAIT);
}

void
i386_bus_space_mallocok(void)
{
	ioport_malloc_safe = 1;
}

void
i386_bus_space_check(avail_end, biosbasemem, biosextmem)
	vm_offset_t avail_end;
	int biosbasemem, biosextmem;
{
	/*
	 * Use BIOS values passed in from the boot program.
	 *
	 * XXX Not only does probing break certain 386 AT relics, but
	 * not all BIOSes (Dell, Compaq, others) report the correct
	 * amount of extended memory.
	 */
	avail_end = biosextmem ? ISA_HOLE_END + biosextmem * 1024 : biosbasemem * 1024; /* just temporary use */

	/*
	 * Allocate the physical addresses used by RAM from the iomem
	 * extent map.  This is done before the addresses are
	 * page rounded just to make sure we get them all.
	 */
	if (extent_alloc_region(iomem_ex, 0, ISA_HOLE_START, EX_NOWAIT)) {
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE BASE MEMORY FROM IOMEM EXTENT MAP!\n");
	}
	if (avail_end > ISA_HOLE_END && extent_alloc_region(iomem_ex, ISA_HOLE_END,
	    (avail_end - ISA_HOLE_END), EX_NOWAIT)) {
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE EXTENDED MEMORY FROM IOMEM EXTENT MAP!\n");
	}
}

int
i386_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int error;
	struct extent *ex;

	/*
	 * Pick the appropriate extent map.
	 */
	if (t == I386_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (t == I386_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("i386_memio_map: bad bus space tag");

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(ex, bpa, size, EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0));
	if (error)
		return (error);

	/*
	 * For I/O space, that's all she wrote.
	 */
	if (t == I386_BUS_SPACE_IO) {
		*bshp = bpa;
		return (0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = i386_mem_add_mapping(bpa, size,
		(flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp);
	if (error) {
		if (extent_free(ex, bpa, size, EX_NOWAIT |
		    (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("i386_memio_map: pa 0x%lx, size 0x%lx\n",
			    bpa, size);
			printf("i386_memio_map: can't free region\n");
		}
	}

	return (error);
}

int
_i386_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{

	/*
	 * For I/O space, just fill in the handle.
	 */
	if (t == I386_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		*bshp = bpa;
		return (0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	return (i386_mem_add_mapping(bpa, size,
			(flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp));
}

int
i386_memio_alloc(t, rstart, rend, size, alignment, boundary, flags, bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	struct extent *ex;
	u_long bpa;
	int error;

	/*
	 * Pick the appropriate extent map.
	 */
	if (t == I386_BUS_SPACE_IO) {
		if (flags & BUS_SPACE_MAP_LINEAR)
			return (EOPNOTSUPP);
		ex = ioport_ex;
	} else if (t == I386_BUS_SPACE_MEM)
		ex = iomem_ex;
	else
		panic("i386_memio_alloc: bad bus space tag");

	/*
	 * Sanity check the allocation against the extent's boundaries.
	 */
	if (rstart < ex->ex_start || rend > ex->ex_end)
		panic("i386_memio_alloc: bad region start/end");

	/*
	 * Do the requested allocation.
	 */
	error = extent_alloc_subregion(ex, rstart, rend, size, alignment, boundary,
	EX_FAST | EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0), &bpa);

	if (error)
		return (error);

	/*
	 * For I/O space, that's all she wrote.
	 */
	if (t == I386_BUS_SPACE_IO) {
		*bshp = *bpap = bpa;
		return (0);
	}

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = i386_mem_add_mapping(bpa, size,
			(flags & BUS_SPACE_MAP_CACHEABLE) != 0, bshp);
	if (error) {
		if (extent_free(iomem_ex, bpa, size,
				EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("i386_memio_alloc: pa 0x%lx, size 0x%lx\n", bpa, size);
			printf("i386_memio_alloc: can't free region\n");
		}
	}

	*bpap = bpa;

	return (error);
}

int
i386_mem_add_mapping(bpa, size, cacheable, bshp)
	bus_addr_t bpa;
	bus_size_t size;
	int cacheable;
	bus_space_handle_t *bshp;
{
	u_long pa, endpa;
	vm_offset_t va;

	pa = i386_trunc_page(bpa);
	endpa = i386_round_page(bpa + size);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("i386_mem_add_mapping: overflow");
#endif

	va = kmem_alloc_pageable(kernel_map, endpa - pa);
	if (va == 0)
		return (ENOMEM);

	*bshp = (bus_space_handle_t)(va + (bpa & PGOFSET));

	for (; pa < endpa; pa += NBPG, va += NBPG) {
		pmap_enter(pmap_kernel(), va, pa, VM_PROT_READ | VM_PROT_WRITE, TRUE);
#if 0
		/*
		 * Not done for two reasons:
		 *
		 *	(1) PG_N doesn't exist on the 386.
		 *
		 *	(2) pmap_changebit() only deals with
		 *	    managed pages.
		 */
		if (!cacheable)
			pmap_changebit(pa, PG_N, ~0);
		else
			pmap_changebit(pa, 0, ~PG_N);
#endif
	}

	return 0;
}

void
i386_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	struct extent *ex;
	u_long va, endva;
	bus_addr_t bpa;

	/*
	 * Find the correct extent and bus physical address.
	 */
	if (t == I386_BUS_SPACE_IO) {
		ex = ioport_ex;
		bpa = bsh;
	} else if (t == I386_BUS_SPACE_MEM) {
		ex = iomem_ex;
		va = i386_trunc_page(bsh);
		endva = i386_round_page(bsh + size);

#ifdef DIAGNOSTIC
		if (endva <= va)
			panic("i386_memio_unmap: overflow");
#endif

		bpa = pmap_extract(pmap_kernel(), va) + (bsh & PGOFSET);

		/*
		 * Free the kernel virtual mapping.
		 */
		kmem_free(kernel_map, va, endva - va);
	} else
		panic("i386_memio_unmap: bad bus space tag");

	if (extent_free(ex, bpa, size, EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("i386_memio_unmap: %s 0x%lx, size 0x%lx\n",
				(t == I386_BUS_SPACE_IO) ? "port" : "pa", bpa, size);
		printf("i386_memio_unmap: can't free region\n");
	}
}

void
i386_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	/* i386_memio_unmap() does all that we need to do. */
	i386_memio_unmap(t, bsh, size);
}

int
i386_memio_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{
	*nbshp = bsh + offset;
	return (0);
}

/*
 * Utility function to load a linear buffer.
 */
static int
bus_dmamap_load_buffer(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct vmspace *vm, int flags)
{
	bus_size_t sgsize;
	bus_addr_t curaddr;
	caddr_t vaddr = (caddr_t)buf;
	pmap_t pmap;

	if (vm != NULL)
		pmap = vm_map_pmap(&vm->vm_map);
	else
		pmap = pmap_kernel();

	while (buflen > 0) {
		int error;

		/*
		 * Get the bus address for this segment.
		 */
		curaddr = _BUS_VIRT_TO_BUS(pmap, vaddr);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = PAGE_SIZE - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		/*
		 * If we're beyond the bounce threshold, notify
		 * the caller.
		 */
		if (map->_dm_bounce_thresh != 0 &&
		    curaddr + sgsize >= map->_dm_bounce_thresh)
			return (EINVAL);


		error = bus_dmamap_load_busaddr(t, map, curaddr, sgsize);
		if (error)
			return error;

		vaddr += sgsize;
		buflen -= sgsize;
	}

	return (0);
}


/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct i386_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct i386_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	bzero(mapstore, mapsize);
	map = (struct i386_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxmaxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_nsegs = 0;		/* no valid mappings */

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	free(map, M_DEVBUF);
}

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	caddr_t vaddr = buf;
	int first, seg;
	pmap_t pmap;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	lastaddr = ~0;		/* XXX gcc */
	bmask  = ~(map->_dm_boundary - 1);

	for (first = 1, seg = 0; buflen > 0; ) {
		/*
		 * Get the physical address for this segment.
		 */
		curaddr = (bus_addr_t)pmap_extract(pmap, (vm_offset_t)vaddr);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = NBPG - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		/*
		 * Make sure we don't cross any boundaries.
		 */
		if (map->_dm_boundary > 0) {
			baddr = (curaddr + map->_dm_boundary) & bmask;
			if (sgsize > (baddr - curaddr))
				sgsize = (baddr - curaddr);
		}

		/*
		 * Insert chunk into a segment, coalescing with
		 * previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->_dm_maxsegsz &&
			     (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr = curaddr;
				map->dm_segs[seg].ds_len = sgsize;
			}
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	/*
	 * Did we fit?
	 */
	if (buflen != 0)
		return (EFBIG);		/* XXX better return value here? */

	map->dm_nsegs = seg + 1;
	return (0);
}

static int
bus_dmamap_load_busaddr(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t addr, bus_size_t size)
{
	bus_dma_segment_t * const segs = map->dm_segs;
	int nseg = map->dm_nsegs;
	bus_addr_t bmask = ~(map->_dm_boundary - 1);
	bus_addr_t lastaddr = 0xdead; /* XXX gcc */
	bus_size_t sgsize;

	if (nseg > 0)
		lastaddr = segs[nseg-1].ds_addr + segs[nseg-1].ds_len;
again:
	sgsize = size;
	/*
	 * Make sure we don't cross any boundaries.
	 */
	if (map->_dm_boundary > 0) {
		bus_addr_t baddr; /* next boundary address */

		baddr = (addr + map->_dm_boundary) & bmask;
		if (sgsize > (baddr - addr))
			sgsize = (baddr - addr);
	}

	/*
	 * Insert chunk into a segment, coalescing with
	 * previous segment if possible.
	 */
	if (nseg > 0 && addr == lastaddr &&
	    segs[nseg-1].ds_len + sgsize <= map->dm_maxsegsz &&
	    (map->_dm_boundary == 0 ||
	     (segs[nseg-1].ds_addr & bmask) == (addr & bmask))) {
		/* coalesce */
		segs[nseg-1].ds_len += sgsize;
	} else if (nseg >= map->_dm_segcnt) {
		return EFBIG;
	} else {
		/* new segment */
		segs[nseg].ds_addr = addr;
		segs[nseg].ds_len = sgsize;
		nseg++;
	}

	lastaddr = addr + sgsize;
	if (map->_dm_bounce_thresh != 0 && lastaddr > map->_dm_bounce_thresh)
		return EINVAL;

	addr += sgsize;
	size -= sgsize;
	if (size > 0)
		goto again;

	map->dm_nsegs = nseg;

	return (0);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
bus_dmamap_load_mbuf(t, map, m, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m;
	int flags;
{

	panic("bus_dmamap_load_mbuf: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
	panic("bus_dmamap_load_uio: not implemented");
}

/*
 * Like bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
bus_dmamap_load_raw(t, map, segs, nsegs, size0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size0;
	int flags;
{
	bus_size_t size;
	int i, error = 0;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
	KASSERT(map->dm_maxsegsz <= map->_dm_maxmaxsegsz);

	if (size0 > map->_dm_size)
		return EINVAL;

	for (i = 0, size = size0; i < nsegs && size > 0; i++) {
		bus_dma_segment_t *ds = &segs[i];
		bus_size_t sgsize;

		sgsize = min(ds->ds_len, size);
		if (sgsize == 0)
			continue;
		error = bus_dmamap_load_busaddr(t, map, ds->ds_addr, sgsize);
		if (error != 0)
			break;
		size -= sgsize;
	}

	if (error != 0) {
		map->dm_mapsize = 0;
		map->dm_nsegs = 0;
		return error;
	}

	/* XXX TBD bounce */

	map->dm_mapsize = size0;
	return (0);
	//panic("bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * bus-specific DMA map unload functions.
 */
void
bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	/*
	 * No resources to free; just mark the mappings as
	 * invalid.
	 */
	map->dm_maxsegsz = map->_dm_maxmaxsegsz;
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void
bus_dmamap_sync(t, map, op)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dmasync_op_t op;
{
	/* Nothing to do here. */
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	return (bus_dmamem_alloc_range(t, size, alignment, boundary, segs, nsegs, rsegs, flags, 0, trunc_page(avail_end)));
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	vm_page_t m;
	bus_addr_t addr;
	struct pglist mlist;
	int curseg;

	/*
	 * Build a list of pages to free back to the VM system.
	 */
	TAILQ_INIT(&mlist);
	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += PAGE_SIZE) {
			m = PHYS_TO_VM_PAGE(addr);
			TAILQ_INSERT_TAIL(&mlist, m, pageq);
		}
	}

	vm_page_free_memory(&mlist);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t 		t;
	bus_dma_segment_t 	*segs;
	int 				nsegs;
	size_t 				size;
	caddr_t 			*kvap;
	int 				flags;
{
	vm_offset_t va;
	bus_addr_t addr;
	int curseg, s;

	size = round_page(size);

	s = splimp();
	va = kmem_alloc_pageable(kmem_map, size);
	splx(s);

	if (va == 0)
		return (ENOMEM);

	*kvap = (caddr_t)va;

	for (curseg = 0; curseg < nsegs; curseg++) {
		for (addr = segs[curseg].ds_addr;
		    addr < (segs[curseg].ds_addr + segs[curseg].ds_len);
		    addr += NBPG, va += NBPG, size -= NBPG) {
			if (size == 0)
				panic("_bus_dmamem_map: size botch");
			pmap_enter(pmap_kernel(), va, addr, VM_PROT_READ | VM_PROT_WRITE, TRUE);
#if 0
			/*
			 * This is not necessary on x86-family
			 * processors.
			 */
			if (flags & BUS_DMAMEM_NOSYNC)
				pmap_changebit(addr, PG_N, ~0);
			else
				pmap_changebit(addr, 0, ~PG_N);
#endif
		}
	}

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
bus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t 	t;
	caddr_t 		kva;
	size_t 			size;
{
	int s;

#ifdef DIAGNOSTIC
	if ((u_long)kva & PGOFSET)
		panic("bus_dmamem_unmap");
#endif

	size = round_page(size);
	s = splimp();
	kmem_free(kmem_map, (vm_offset_t)kva, size);
	splx(s);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
int
bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t 		t;
	bus_dma_segment_t 	*segs;
	int 				nsegs, off, prot, flags;
{
	int i;

	for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
		if (off & PGOFSET)
			panic("bus_dmamem_mmap: offset unaligned");
		if (segs[i].ds_addr & PGOFSET)
			panic("bus_dmamem_mmap: segment unaligned");
		if (segs[i].ds_len & PGOFSET)
			panic("bus_dmamem_mmap: segment size not multiple"
			    " of page size");
#endif
		if (off >= segs[i].ds_len) {
			off -= segs[i].ds_len;
			continue;
		}

		return (i386_btop((caddr_t)segs[i].ds_addr + off));
	}

	/* Page not found. */
	return (-1);
}

/**********************************************************************
 * DMA utility functions
 **********************************************************************/

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
bus_dmamem_alloc_range(t, size, alignment, boundary, segs, nsegs, rsegs,
    flags, low, high)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
	vm_offset_t low;
	vm_offset_t high;
{
	vm_offset_t curaddr, lastaddr;
	vm_page_t m;
	struct pglist mlist;
	int curseg, error;

	/* Always round the size. */
	size = round_page(size);

	/*
	 * Allocate pages from the VM system.
	 */
	TAILQ_INIT(&mlist);
	error = vm_page_alloc_memory(size, low, high, alignment, boundary, &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0); /* uvm_pglistalloc */

	if (error)
		return (error);

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	m = TAILQ_FIRST(mlist);
	curseg = 0;
	lastaddr = segs[curseg].ds_addr = VM_PAGE_TO_PHYS(m);
	segs[curseg].ds_len = PAGE_SIZE;
	m = TAILQ_NEXT(m, pageq);

	for (; m != NULL; m = TAILQ_NEXT(m, pageq)) {
		curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
		if (curaddr < low || curaddr >= high) {
			printf("vm_page_alloc_memory returned non-sensical"
			    " address 0x%lx\n", curaddr);
			panic("_bus_dmamem_alloc_range");
		}
#endif
		if (curaddr == (lastaddr + PAGE_SIZE))
			segs[curseg].ds_len += PAGE_SIZE;
		else {
			curseg++;
			segs[curseg].ds_addr = curaddr;
			segs[curseg].ds_len = PAGE_SIZE;
		}
		lastaddr = curaddr;
	}

	*rsegs = curseg + 1;

	return (0);
}
