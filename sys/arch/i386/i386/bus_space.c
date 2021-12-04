/*	$NetBSD: bus_space.c,v 1.45 2020/04/25 15:26:18 bouyer Exp $	*/

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
/* __KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.45 2020/04/25 15:26:18 bouyer Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/user.h>
#include <sys/bus.h>

#include <vm/include/vm_extern.h>

#include <dev/core/isa/isareg.h>

#include <i386/isa/isa_machdep.h>

#include <machine/cpufunc.h>
#include <machine/pio.h>
#include <machine/bus_dma.h>
#include <machine/bus_space.h>

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
	return(i386_memio_alloc(t, rs, re, s, a, b, f, ap, hp));
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

	return((tag == I386_BUS_SPACE_MEM ? (void *)(bsh) : (void *)0));
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

	if (t == I386_BUS_SPACE_IO)
		while (c--)
			outl(addr, v);
	else
		while (c--)
			*(volatile u_int32_t*) (addr) = v;
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
