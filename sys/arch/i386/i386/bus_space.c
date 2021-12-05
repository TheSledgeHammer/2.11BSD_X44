/*	$NetBSD: bus_space.c,v 1.2 2003/03/14 18:47:53 christos Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

#include <i386/isa/isa_machdep.h>

#include <machine/pmap.h>
#include <machine/pmap_reg.h>

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
	pt_entry_t *pte;
	//int32_t cpumask = 0;

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
		pmap_enter(kernel_pmap, va, pa, VM_PROT_READ | VM_PROT_WRITE, TRUE);

		if (cpu_class != CPUCLASS_386) {
			pte = kvtopte(va);
			if (cacheable) {
				*pte &= ~PG_N;
			} else {
				*pte |= PG_N;
			}
			pmap_invalidate_page(kernel_pmap, va);
		}
	}

	pmap_update(kernel_pmap);
	return (0);
}

/*
 * void _i386_memio_unmap(bus_space_tag bst, bus_space_handle bsh,
 *                        bus_size_t size, bus_addr_t *adrp)
 *
 *   This function unmaps memory- or io-space mapped by the function
 *   _x86_memio_map().  This function works nearly as same as
 *   x86_memio_unmap(), but this function does not ask kernel
 *   built-in extents and returns physical address of the bus space,
 *   for the convenience of the extra extent manager.
 */
void
_i386_memio_unmap(t, bsh, size, adrp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
	bus_addr_t *adrp;
{
	u_long va, endva;
	bus_addr_t bpa;

	/*
	 * Find the correct extent and bus physical address.
	 */
	if (t == I386_BUS_SPACE_IO) {
		bpa = bsh;
	} else if (t == I386_BUS_SPACE_IO) {
		if (bsh >= ISA_HOLE_START && (bsh + size) <= (ISA_HOLE_END + ISA_HOLE_LENGTH)) {
			bpa = (bus_addr_t) ISA_PHYSADDR(bsh);
		} else {

			va = i386_trunc_page(bsh);
			endva = i386_round_page(bsh + size);

#ifdef DIAGNOSTIC
			if (endva <= va) {
				panic("_x86_memio_unmap: overflow");
			}
#endif
			bpa = pmap_extract(kernel_pmap, va) + (bsh & PGOFSET);

			pmap_kremove(va, endva - va);
			/*
			 * Free the kernel virtual mapping.
			 */
			kmem_free(kernel_map, va, endva - va);
		}
	} else {
		panic("_i386_memio_unmap: bad bus space tag");
	}

	if (adrp != NULL) {
		*adrp = bpa;
	}
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

		bpa = pmap_extract(kernel_pmap, va) + (bsh & PGOFSET);

		pmap_kremove(va, endva - va);

		/*
		 * Free the kernel virtual mapping.
		 */
		kmem_free(kernel_map, va, endva - va);
	} else {
		panic("i386_memio_unmap: bad bus space tag");
	}

	if (extent_free(ex, bpa, size, EX_NOWAIT | (ioport_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("i386_memio_unmap: %s 0x%lx, size 0x%lx\n", (t == I386_BUS_SPACE_IO) ? "port" : "pa", bpa, size);
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

caddr_t
i386_memio_mmap(t, addr, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t addr;
	off_t off;
	int prot;
	int flags;
{

	/* Can't mmap I/O space. */
	if (t == I386_BUS_SPACE_IO) {
		return (-1);
	}

	/*
	 * "addr" is the base address of the device we're mapping.
	 * "off" is the offset into that device.
	 *
	 * Note we are called for each "page" in the device that
	 * the upper layers want to map.
	 */
	return (i386_btop(addr + off));
}

void
i386_memio_set_multi_1(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t v;
	size_t c;
{
	bus_space_set_multi_1(t, h, o, v, c);
}

void
i386_memio_set_multi_2(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t v;
	size_t c;
{
	bus_space_set_multi_2(t, h, o, v, c);
}

void
i386_memio_set_multi_4(t, h, o, v, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h;
	bus_size_t 			o;
	u_int32_t 			v;
	size_t 				c;
{
	bus_space_set_multi_4(t, h, o, v, c);
}

void
i386_memio_set_region_1(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t v;
	size_t c;
{
	bus_space_set_region_1(t, h, o, v, c);
}

void
i386_memio_set_region_2(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t v;
	size_t c;
{
	bus_space_set_region_2(t, h, o, v, c);
}

void
i386_memio_set_region_4(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int32_t v;
	size_t c;
{
	bus_space_set_region_4(t, h, o, v, c);
}

void
i386_memio_copy_region_1(t, h1, o1, h2, o2, c)
	bus_space_tag_t t;
	bus_space_handle_t h1;
	bus_size_t o1;
	bus_space_handle_t h2;
	bus_size_t o2;
	size_t c;
{
	bus_space_copy_region_1(t, h1, o1, h2, o2, c);
}

void
i386_memio_copy_region_2(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	bus_space_copy_region_2(t, h1, o1, h2, o2, c);
}

void
i386_memio_copy_region_4(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	bus_space_copy_region_4(t, h1, o1, h2, o2, c);
}

/*
 *	int bus_space_map  __P((bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp));
 *
 * Map a region of bus space.
 */
int
bus_space_map(t, a, s, f, hp)
	bus_space_tag_t t;
	bus_addr_t a;
	bus_size_t s;
	int f;
	bus_space_handle_t *hp;
{
	return (i386_memio_map(t, a, s, f, hp));
}

/*
 *	int bus_space_unmap __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * Unmap a region of bus space.
 */
void
bus_space_unmap(t, h, s)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t s;
{
	i386_memio_unmap(t, h, s);
}

/*
 *	int bus_space_subregion __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp));
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */
int
bus_space_subregion(t, h, o, s, nhp)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	bus_size_t s;
	bus_space_handle_t *nhp;
{
	return (i386_memio_subregion(t, h, o, s, nhp));
}

/*
 *	int bus_space_alloc __P((bus_space_tag_t t, bus_addr_t rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *addrp,
 *	    bus_space_handle_t *bshp));
 *
 * Allocate a region of bus space.
 */
int
bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)
	bus_space_tag_t t;
	bus_addr_t rs, re, *ap;
	bus_size_t s, a, b;
	int f;
	bus_space_handle_t *hp;
{
	return (i386_memio_alloc(t, rs, re, s, a, b, f, ap, hp));
}

/*
 *	int bus_space_free __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * Free a region of bus space.
 */
void
bus_space_free(t, h, s)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t s;
{
	i386_memio_free(t, h, s);
}

caddr_t
bus_space_mmap(t, addr, off, prot, flags)
	bus_space_tag_t t;
	bus_addr_t addr;
	off_t off;
	int prot;
	int flags;
{
	//panic("bus_space_mmap: not implemented");

	return (i386_memio_mmap(t, addr, off, prot, flags));
}

void *
bus_space_vaddr(tag, bsh)
	bus_space_tag_t	tag;
	bus_space_handle_t bsh;
{
	//panic("bus_space_vaddr: not implemented");

	return ((tag == I386_BUS_SPACE_MEM ? (void *)(bsh) : (void *)0));
}

/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

u_int8_t
bus_space_read_1(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	return (t == I386_BUS_SPACE_IO ? (inb(h + o)) : (*(volatile u_int8_t *)(h + o)));
}

u_int16_t
bus_space_read_2(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	return (__BUS_SPACE_ADDRESS_SANITY(h + o, u_int16_t, "bus addr"), (t == I386_BUS_SPACE_IO ? (inw(h + o)) : (*(volatile u_int16_t *)(h + o))));
}

u_int32_t
bus_space_read_4(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	return (__BUS_SPACE_ADDRESS_SANITY(h + o, u_int32_t, "bus addr"), (t == I386_BUS_SPACE_IO ? (inl(h + o)) : (*(volatile u_int32_t *)(h + o))));
}

#if 0	/* Cause a link error for bus_space_read_8 */
u_int64_t
bus_space_read_8(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	panic("!!! bus_space_read_8 not implemented !!!");
}
#endif

u_int8_t
bus_space_read_stream_1(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	return (bus_space_read_1(t, h, o));
}

u_int16_t
bus_space_read_stream_2(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	return (bus_space_read_2(t, h, o));
}

u_int32_t
bus_space_read_stream_4(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	return (bus_space_read_4(t, h, o));
}

#if 0	/* Cause a link error for bus_space_read_stream_8 */
u_int64_t
bus_space_read_stream_8(t, h, o)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
{
	panic("!!! bus_space_read_stream_8 not implemented !!!");
}
#endif
/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

void
bus_space_read_multi_1(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t *ptr;
	size_t cnt;
{
	if ((t) == I386_BUS_SPACE_IO) {
		insb((h) + (o), (ptr), (cnt));
	} else {
		void *dummy1;
		int dummy2;
		void *dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	movb (%2),%%al										;	\
				stosb												;	\
				loop 1b" :
				"=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) :
				"0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)) :
				"memory");
	}
}

void
bus_space_read_multi_2(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t *ptr;
	size_t cnt;
{
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		insw((h) + (o), (ptr), (cnt));
	} else {
		void *dummy1;
		int dummy2;
		void *dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	movw (%2),%%ax										;	\
				stosw												;	\
				loop 1b" :
				"=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) :
				"0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)) :
				"memory");
	}
}

void
bus_space_read_multi_4(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int32_t *ptr;
	size_t cnt;
{
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		insl((h) + (o), (ptr), (cnt));
	} else {
		void *dummy1;
		int dummy2;
		void *dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	movl (%2),%%eax										;	\
				stosl												;	\
				loop 1b" :
				"=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) :
				"0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)) :
				"memory");
	}
}

#if 0	/* Cause a link error for bus_space_read_multi_8 */
void
bus_space_read_multi_8(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int64_t *ptr;
	size_t cnt;
{
	panic("!!! bus_space_read_multi_8 not implemented !!!");
}
#endif

void
bus_space_read_multi_stream_1(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t *ptr;
	size_t cnt;
{
	bus_space_read_multi_1(t, h, o, ptr, cnt);
}

void
bus_space_read_multi_stream_2(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t *ptr;
	size_t cnt;
{
	bus_space_read_multi_2(t, h, o, ptr, cnt);
}

void
bus_space_read_multi_stream_4(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int32_t *ptr;
	size_t cnt;
{
	bus_space_read_multi_4(t, h, o, ptr, cnt);
}

#if 0	/* Cause a link error for bus_space_read_multi_stream_8 */
void
bus_space_read_multi_stream_8(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int64_t *ptr;
	size_t cnt;
{
	panic("!!! bus_space_read_multi_stream_8 not implemented !!!");
}
#endif

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

void
bus_space_read_region_1(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int8_t *ptr;
	size_t cnt;
{
	if ((t) == I386_BUS_SPACE_IO) {
		int dummy1;
		void *dummy2;
		int dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	inb %w1,%%al										;	\
				stosb												;	\
				incl %1												;	\
				loop 1b" :
				"=&a" (__x), "=d" (dummy1), "=D" (dummy2),
				"=c" (dummy3) :
				"1" ((h) + (o)), "2" ((ptr)), "3" ((cnt)) :
				"memory");
	} else {
		int dummy1;
		void *dummy2;
		int dummy3;
		__asm __volatile("											\
				cld													;	\
				repne												;	\
				movsb" :
				"=S" (dummy1), "=D" (dummy2), "=c" (dummy3) :
				"0" ((h) + (o)), "1" ((ptr)), "2" ((cnt)) :
				"memory");
	}
}

void
bus_space_read_region_2(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int16_t *ptr;
	size_t cnt;
{
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		int dummy1;
		void *dummy2;
		int dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	inw %w1,%%ax										;	\
				stosw												;	\
				addl $2,%1											;	\
				loop 1b" :
				"=&a" (__x), "=d" (dummy1), "=D" (dummy2),
				"=c" (dummy3)
				"1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))
				"memory");
	} else {
		int dummy1;
		void *dummy2;
		int dummy3;
		__asm __volatile("											\
				cld													;	\
				repne												;	\
				movsw"
				"=S" (dummy1), "=D" (dummy2), "=c" (dummy3)
				"0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))
				"memory");
	}
}

void
bus_space_read_region_4(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int32_t *ptr;
	size_t cnt;
{
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		int dummy1;
		void *dummy2;
		int dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	inl %w1,%%eax										;	\
				stosl												;	\
				addl $4,%1											;	\
				loop 1b" :
				"=&a" (__x), "=d" (dummy1), "=D" (dummy2),
				"=c" (dummy3) :
				"1" ((h) + (o)), "2" ((ptr)), "3" ((cnt)) :
				"memory");
	} else {
		int dummy1;
		void *dummy2;
		int dummy3;
		__asm __volatile("											\
				cld													;	\
				repne												;	\
				movsl" :
				"=S" (dummy1), "=D" (dummy2), "=c" (dummy3) :
				"0" ((h) + (o)), "1" ((ptr)), "2" ((cnt)) :
				"memory");
	}
}

#if 0	/* Cause a link error for bus_space_read_region_8 */
void
bus_space_read_region_8(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int64_t *ptr;
	size_t cnt;
{
	panic("!!! bus_space_read_region_8 not implemented !!!");
}
#endif

void
bus_space_read_region_stream_1(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int8_t *ptr;
	size_t cnt;
{
	bus_space_read_region_1(t, h, o, ptr, cnt);
}

void
bus_space_read_region_stream_2(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int16_t *ptr;
	size_t cnt;
{
	bus_space_read_region_2(t, h, o, ptr, cnt);
}

void
bus_space_read_region_stream_4(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int32_t *ptr;
	size_t cnt;
{
	bus_space_read_region_4(t, h, o, ptr, cnt);
}

#if 0	/* Cause a link error for bus_space_read_region_stream_8 */
void
bus_space_read_region_stream_8(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int64_t *ptr;
	size_t cnt;
{
	panic("!!! bus_space_read_region_stream_8 not implemented !!!");
}
#endif

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

void
bus_space_write_1(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t v;
{
	if ((t) == I386_BUS_SPACE_IO) {
		outb((h) + (o), (v));
	} else {
		((void) (*(volatile u_int8_t*) ((h) + (o)) = (v)));
	}
}

void
bus_space_write_2(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t v;
{
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		outw((h) + (o), (v));
	} else {
		((void) (*(volatile u_int16_t*) ((h) + (o)) = (v)));
	}
}

void
bus_space_write_4(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int32_t v;
{
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		outl((h) + (o), (v));
	} else {
		((void) (*(volatile u_int32_t*) ((h) + (o)) = (v)));
	}
}

#if 0	/* Cause a link error for bus_space_write_8 */
void
bus_space_write_8(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int64_t v;
{
	panic("!!! bus_space_write_8 not implemented !!!");
}
#endif

void
bus_space_write_stream_1(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t v;
{
	bus_space_write_1(t, h, o, v);
}

void
bus_space_write_stream_2(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t v;
{
	bus_space_write_2(t, h, o, v);
}

void
bus_space_write_stream_4(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int32_t v;
{
	bus_space_write_4(t, h, o, v);
}

#if 0	/* Cause a link error for bus_space_write_stream_8 */
void
bus_space_write_stream_8(t, h, o, v)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int64_t v;
{
	panic("!!! bus_space_write_stream_8 not implemented !!!");
}
#endif

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

void
bus_space_write_multi_1(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int8_t *ptr;
	size_t cnt;
{
	if ((t) == I386_BUS_SPACE_IO) {
		outsb((h) + (o), (ptr), (cnt));
	} else {
		void *dummy1;
		int dummy2;
		void *dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	lodsb												;	\
				movb %%al,(%2)										;	\
				loop 1b" :
				"=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) :
				"0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));
	}
}

void
bus_space_write_multi_2(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int16_t *ptr;
	size_t cnt;
{
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		outsw((h) + (o), (ptr), (cnt));
	} else {
		void *dummy1;
		int dummy2;
		void *dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	lodsw												;	\
				movw %%ax,(%2)										;	\
				loop 1b" :
				"=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) :
				"0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));
	}
}

void
bus_space_write_multi_4(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int32_t *ptr;
	size_t cnt;
{
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		outsl((h) + (o), (ptr), (cnt));
	} else {
		void *dummy1;
		int dummy2;
		void *dummy3;
		int __x;
		__asm __volatile("											\
				cld													;	\
			1:	lodsl												;	\
				movl %%eax,(%2)										;	\
				loop 1b" :
				"=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) :
				"0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));
	}
}

#if 0	/* Cause a link error for bus_space_write_multi_8 */
void
bus_space_write_multi_8(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int64_t *ptr;
	size_t cnt;
{
	panic("!!! bus_space_write_multi_8 not implemented !!!");
}
#endif

void
bus_space_write_multi_stream_1(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int8_t *ptr;
	size_t cnt;
{
	bus_space_write_multi_1(t, h, o, ptr, cnt);
}

void
bus_space_write_multi_stream_2(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int16_t *ptr;
	size_t cnt;
{
	bus_space_write_multi_2(t, h, o, ptr, cnt);
}

void
bus_space_write_multi_stream_4(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int32_t *ptr;
	size_t cnt;
{
	bus_space_write_multi_4(t, h, o, ptr, cnt);
}

#if 0	/* Cause a link error for bus_space_write_multi_stream_8 */
void
bus_space_write_multi_stream_8(t, h, o, ptr, cnt)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int64_t *ptr;
	size_t cnt;
{
	panic("!!! bus_space_write_multi_stream_8 not implemented !!!");
}
#endif

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

void
bus_space_write_region_1(t, h, o, ptr, cnt)
	bus_space_tag_t	t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int8_t *ptr;
	size_t cnt;
{
	if ((t) == I386_BUS_SPACE_IO) {
		int dummy1;
		void *dummy2;
		int dummy3;
		int __x;
		__asm __volatile("											\
			cld													;	\
		1:	lodsb												;	\
			outb %%al,%w1										;	\
			incl %1												;	\
			loop 1b" :
				"=&a" (__x), "=d" (dummy1), "=S" (dummy2),
				"=c" (dummy3) :
				"1" ((h) + (o)), "2" ((ptr)), "3" ((cnt)) :
				"memory");
	} else {
		int dummy1;
		void *dummy2;
		int dummy3;
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsb" :
				"=D" (dummy1), "=S" (dummy2), "=c" (dummy3) :
				"0" ((h) + (o)), "1" ((ptr)), "2" ((cnt)) :
				"memory");
	}
}

void
bus_space_write_region_2(t, h, o, ptr, cnt)
	bus_space_tag_t	t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int16_t *ptr;
	size_t cnt;
{
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		int dummy1;
		void *dummy2;
		int dummy3;
		int __x;
		__asm __volatile("											\
					cld													;	\
				1:	lodsw												;	\
					outw %%ax,%w1										;	\
					addl $2,%1											;	\
					loop 1b" :
				"=&a" (__x), "=d" (dummy1), "=S" (dummy2),
				"=c" (dummy3) :
				"1" ((h) + (o)), "2" ((ptr)), "3" ((cnt)) :
				"memory");
	} else {
		int dummy1;
		void *dummy2;
		int dummy3;
		__asm __volatile("											\
					cld													;	\
					repne												;	\
					movsw" :
				"=D" (dummy1), "=S" (dummy2), "=c" (dummy3) :
				"0" ((h) + (o)), "1" ((ptr)), "2" ((cnt)) :
				"memory");
	}
}

void
bus_space_write_region_4(t, h, o, ptr, cnt)
	bus_space_tag_t	t;
	bus_space_handle_t h;
	bus_size_t o;
	const u_int32_t *ptr;
	size_t cnt;
{
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");
	if ((t) == I386_BUS_SPACE_IO) {
		int dummy1;
		void *dummy2;
		int dummy3;
		int __x;
		__asm __volatile("											\
					cld													;	\
				1:	lodsl												;	\
					outl %%eax,%w1										;	\
					addl $4,%1											;	\
					loop 1b" :
				"=&a" (__x), "=d" (dummy1), "=S" (dummy2),
				"=c" (dummy3) :
				"1" ((h) + (o)), "2" ((ptr)), "3" ((cnt)) :
				"memory");
	} else {
		int dummy1;
		void *dummy2;
		int dummy3;
		__asm __volatile("											\
					cld													;	\
					repne												;	\
					movsl" :
				"=D" (dummy1), "=S" (dummy2), "=c" (dummy3) :
				"0" ((h) + (o)), "1" ((ptr)), "2" ((cnt)) :
				"memory");
	}
}

/*
 *	void bus_space_set_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count));
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */
void
bus_space_set_multi_1(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t v;
	size_t c;
{
	bus_addr_t addr = h + o;

	if (t == I386_BUS_SPACE_IO) {
		while (c--) {
			outb(addr, v);
		}
	} else {
		while (c--) {
			*(volatile u_int8_t*) (addr) = v;
		}
	}
}

void
bus_space_set_multi_2(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t v;
	size_t c;
{
	bus_addr_t addr = h + o;

	if (t == I386_BUS_SPACE_IO) {
		while (c--) {
			outw(addr, v);
		}
	} else {
		while (c--) {
			*(volatile u_int16_t*) (addr) = v;
		}
	}
}

void
bus_space_set_multi_4(t, h, o, v, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h;
	bus_size_t 			o;
	u_int32_t 			v;
	size_t 				c;
{
	bus_addr_t addr = h + o;

	if (t == I386_BUS_SPACE_IO) {
		while (c--) {
			outl(addr, v);
		}
	} else {
		while (c--) {
			*(volatile u_int32_t*) (addr) = v;
		}
	}
}

#if 0	/* Cause a link error for bus_space_set_multi_8 */
void
bus_space_set_multi_8(t, h, o, v, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h;
	bus_size_t 			o;
	u_int64_t 			v;
	size_t 				c;
{
	panic("!!! bus_space_set_multi_8 unimplemented !!!");
}
#endif

/*
 *	void bus_space_set_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
void
bus_space_set_region_1(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t v;
	size_t c;
{
	bus_addr_t addr = h + o;

	if (t == I386_BUS_SPACE_IO) {
		for (; c != 0; c--, addr++) {
			outb(addr, v);
		}
	} else {
		for (; c != 0; c--, addr++) {
			*(volatile u_int8_t*) (addr) = v;
		}
	}
}

void
bus_space_set_region_2(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int16_t v;
	size_t c;
{
	bus_addr_t addr = h + o;

	if (t == I386_BUS_SPACE_IO) {
		for (; c != 0; c--, addr += 2) {
			outw(addr, v);
		}
	} else {
		for (; c != 0; c--, addr += 2) {
			*(volatile u_int16_t*) (addr) = v;
		}
	}
}

void
bus_space_set_region_4(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int32_t v;
	size_t c;
{
	bus_addr_t addr = h + o;

	if (t == I386_BUS_SPACE_IO) {
		for (; c != 0; c--, addr += 4) {
			outl(addr, v);
		}
	} else {
		for (; c != 0; c--, addr += 4) {
			*(volatile u_int32_t*) (addr) = v;
		}
	}
}

#if 0	/* Cause a link error for bus_space_set_region_8 */
void
bus_space_set_region_8(t, h, o, v, c)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t o;
	u_int64_t v;
	size_t c;
{
	panic("!!! bus_space_set_region_8 unimplemented !!!");
}
#endif

/*
 *	void bus_space_copy_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count));
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

void
bus_space_copy_region_1(t, h1, o1, h2, o2, c)
	bus_space_tag_t t;
	bus_space_handle_t h1;
	bus_size_t o1;
	bus_space_handle_t h2;
	bus_size_t o2;
	size_t c;
{
	bus_addr_t addr1 = h1 + o1;
	bus_addr_t addr2 = h2 + o2;

	if (t == I386_BUS_SPACE_IO) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1++, addr2++)
				outb(addr2, inb(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += (c - 1), addr2 += (c - 1); c != 0;
					c--, addr1--, addr2--)
				outb(addr2, inb(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1++, addr2++)
				*(volatile u_int8_t*) (addr2) = *(volatile u_int8_t*) (addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += (c - 1), addr2 += (c - 1); c != 0;
					c--, addr1--, addr2--)
				*(volatile u_int8_t*) (addr2) = *(volatile u_int8_t*) (addr1);
		}
	}
}

void
bus_space_copy_region_2(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	bus_addr_t addr1 = h1 + o1;
	bus_addr_t addr2 = h2 + o2;

	if (t == I386_BUS_SPACE_IO) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 2, addr2 += 2)
				outw(addr2, inw(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 2 * (c - 1), addr2 += 2 * (c - 1); c != 0;
					c--, addr1 -= 2, addr2 -= 2)
				outw(addr2, inw(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 2, addr2 += 2)
				*(volatile u_int16_t*) (addr2) = *(volatile u_int16_t*) (addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 2 * (c - 1), addr2 += 2 * (c - 1); c != 0;
					c--, addr1 -= 2, addr2 -= 2)
				*(volatile u_int16_t*) (addr2) = *(volatile u_int16_t*) (addr1);
		}
	}
}

void
bus_space_copy_region_4(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	bus_addr_t addr1 = h1 + o1;
	bus_addr_t addr2 = h2 + o2;

	if (t == I386_BUS_SPACE_IO) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 4, addr2 += 4)
				outl(addr2, inl(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 4 * (c - 1), addr2 += 4 * (c - 1); c != 0;
					c--, addr1 -= 4, addr2 -= 4)
				outl(addr2, inl(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 4, addr2 += 4)
				*(volatile u_int32_t*) (addr2) = *(volatile u_int32_t*) (addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 4 * (c - 1), addr2 += 4 * (c - 1); c != 0;
					c--, addr1 -= 4, addr2 -= 4)
				*(volatile u_int32_t*) (addr2) = *(volatile u_int32_t*) (addr1);
		}
	}
}

#if 0	/* Cause a link error for bus_space_copy_8 */
void
bus_space_copy_region_8(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	panic("!!! bus_space_copy_region_8 unimplemented !!!");
}
#endif

void
bus_space_copy_region_stream_1(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	bus_space_copy_region_1(t, h1, o1, h2, o2, c);
}

void
bus_space_copy_region_stream_2(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	bus_space_copy_region_2(t, h1, o1, h2, o2, c);
}

void
bus_space_copy_region_stream_4(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	bus_space_copy_region_4(t, h1, o1, h2, o2, c);
}

#if 0	/* Cause a link error for bus_space_copy_8 */
void
bus_space_copy_region_stream_8(t, h1, o1, h2, o2, c)
	bus_space_tag_t 	t;
	bus_space_handle_t 	h1;
	bus_size_t 			o1;
	bus_space_handle_t 	h2;
	bus_size_t 			o2;
	size_t 				c;
{
	panic("!!! bus_space_copy_region_stream_8 unimplemented !!!");
}
#endif
