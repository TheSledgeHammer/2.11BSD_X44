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

#ifndef _I386_BUS_SPACE_H_
#define _I386_BUS_SPACE_H_

#include <sys/bus.h>
#include <machine/pio.h>

/*
 * Values for the i386 bus space tag, not to be used directly by MI code.
 */
#define	I386_BUS_SPACE_IO				0	/* space is i/o space */
#define I386_BUS_SPACE_MEM				1	/* space is mem space */

#define __BUS_SPACE_HAS_STREAM_METHODS 	1

void 	i386_bus_space_init	(void);
void 	i386_bus_space_mallocok	(void);
void	i386_bus_space_check (vm_offset_t, int, int);

int		i386_memio_map (bus_space_tag_t t, bus_addr_t addr, bus_size_t size, int flags, bus_space_handle_t *bshp);
/* like map, but without extent map checking/allocation */
int		_i386_memio_map (bus_space_tag_t t, bus_addr_t addr, bus_size_t size, int flags, bus_space_handle_t *bshp);
void 	i386_memio_unmap (bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size);
int		i386_memio_subregion (bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp);
int		i386_memio_alloc (bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend, bus_size_t size, bus_size_t align, bus_size_t boundary, int flags, bus_addr_t *addrp, bus_space_handle_t *bshp);
void 	i386_memio_free (bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size);

/*
 *	void bus_space_read_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */
#define	bus_space_read_multi_1(t, h, o, a, c) do {			\
	if ((t) == I386_BUS_SPACE_IO) {							\
		insb((h) + (o), (a), (c));							\
	} else {												\
		int __x __asm__("%eax");							\
		__asm __volatile("									\
			cld					;							\
		1:	movb (%1),%%al				;					\
			stosb					;						\
			loop 1b"				: 						\
		    "=&a" (__x)					:					\
		    "r" ((h) + (o)), "D" ((a)), "c" ((c))	:		\
		    "%edi", "%ecx", "memory");						\
	}														\
} while (0)

#define	bus_space_read_multi_2(t, h, o, a, c) do {			\
	if ((t) == I386_BUS_SPACE_IO) {							\
		insw((h) + (o), (a), (c));							\
	} else {												\
		int __x __asm__("%eax");							\
		__asm __volatile("									\
			cld					;							\
		1:	movw (%1),%%ax				;					\
			stosw					;						\
			loop 1b"				:						\
		    "=&a" (__x)					:					\
		    "r" ((h) + (o)), "D" ((a)), "c" ((c))	:		\
		    "%edi", "%ecx", "memory");						\
	}														\
} while (0)

#define	bus_space_read_multi_4(t, h, o, a, c) do {			\
	if ((t) == I386_BUS_SPACE_IO) {							\
		insl((h) + (o), (a), (c));							\
	} else {												\
		int __x __asm__("%eax");							\
		__asm __volatile("									\
			cld					;							\
		1:	movl (%1),%%eax				;					\
			stosl					;						\
			loop 1b"				:						\
		    "=&a" (__x)					:					\
		    "r" ((h) + (o)), "D" ((a)), "c" ((c))	:		\
		    "%edi", "%ecx", "memory");						\
	}														\
} while (0)

#if 0	/* Cause a link error for bus_space_read_multi_8 */
#define	bus_space_read_multi_8	!!! bus_space_read_multi_8 unimplemented !!!
#endif

/*
 *	void bus_space_read_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

#define	bus_space_read_region_1(t, h, o, a, c) do {		\
	if ((t) == I386_BUS_SPACE_IO) {						\
		int __x __asm__("%eax");						\
		__asm __volatile("								\
			cld					;						\
		1:	inb %w1,%%al				;				\
			stosb					;					\
			incl %1					;					\
			loop 1b"				: 					\
		    "=&a" (__x)					:				\
		    "d" ((h) + (o)), "D" ((a)), "c" ((c))	:	\
		    "%edx", "%edi", "%ecx", "memory");			\
	} else {											\
		__asm __volatile("								\
			cld					;						\
			repne					;					\
			movsb"					:					\
								:						\
		    "S" ((h) + (o)), "D" ((a)), "c" ((c))	:	\
		    "%esi", "%edi", "%ecx", "memory");			\
	}													\
} while (0)

#define	bus_space_read_region_2(t, h, o, a, c) do {		\
	if ((t) == I386_BUS_SPACE_IO) {						\
		int __x __asm__("%eax");						\
		__asm __volatile("								\
			cld					;						\
		1:	inw %w1,%%ax				;				\
			stosw					;					\
			addl $2,%1				;					\
			loop 1b"				: 					\
		    "=&a" (__x)					:				\
		    "d" ((h) + (o)), "D" ((a)), "c" ((c))	:	\
		    "%edx", "%edi", "%ecx", "memory");			\
	} else {											\
		__asm __volatile("								\
			cld					;						\
			repne					;					\
			movsw"					:					\
								:						\
		    "S" ((h) + (o)), "D" ((a)), "c" ((c))	:	\
		    "%esi", "%edi", "%ecx", "memory");			\
	}													\
} while (0)

#define	bus_space_read_region_4(t, h, o, a, c) do {		\
	if ((t) == I386_BUS_SPACE_IO) {						\
		int __x __asm__("%eax");						\
		__asm __volatile("								\
			cld					;						\
		1:	inl %w1,%%eax				;				\
			stosl					;					\
			addl $4,%1				;					\
			loop 1b"				: 					\
		    "=&a" (__x)					:				\
		    "d" ((h) + (o)), "D" ((a)), "c" ((c))	:	\
		    "%edx", "%edi", "%ecx", "memory");			\
	} else {											\
		__asm __volatile("								\
			cld					;						\
			repne					;					\
			movsl"					:					\
								:						\
		    "S" ((h) + (o)), "D" ((a)), "c" ((c))	:	\
		    "%esi", "%edi", "%ecx", "memory");			\
	}													\
} while (0)

#if 0	/* Cause a link error for bus_space_read_region_8 */
#define	bus_space_read_region_8	!!! bus_space_read_region_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define	bus_space_write_multi_1(t, h, o, a, c) do {			\
	if ((t) == I386_BUS_SPACE_IO) {							\
		outsb((h) + (o), (a), (c));							\
	} else {												\
		int __x __asm__("%eax");							\
		__asm __volatile("									\
			cld					;							\
		1:	lodsb					;						\
			movb %%al,(%1)				;					\
			loop 1b"				: 						\
		    "=&a" (__x)					:					\
		    "r" ((h) + (o)), "S" ((a)), "c" ((c))	:		\
		    "%esi", "%ecx");								\
	}														\
} while (0)

#define bus_space_write_multi_2(t, h, o, a, c) do {			\
	if ((t) == I386_BUS_SPACE_IO) {							\
		outsw((h) + (o), (a), (c));							\
	} else {												\
		int __x __asm__("%eax");							\
		__asm __volatile("									\
			cld					;							\
		1:	lodsw					;						\
			movw %%ax,(%1)				;					\
			loop 1b"				: 						\
		    "=&a" (__x)					:					\
		    "r" ((h) + (o)), "S" ((a)), "c" ((c))	:		\
		    "%esi", "%ecx");								\
	}														\
} while (0)

#define bus_space_write_multi_4(t, h, o, a, c) do {			\
	if ((t) == I386_BUS_SPACE_IO) {							\
		outsl((h) + (o), (a), (c));							\
	} else {												\
		int __x __asm__("%eax");							\
		__asm __volatile("									\
			cld					;							\
		1:	lodsl					;						\
			movl %%eax,(%1)				;					\
			loop 1b"				: 						\
		    "=&a" (__x)					:					\
		    "r" ((h) + (o)), "S" ((a)), "c" ((c))	:		\
		    "%esi", "%ecx");								\
	}														\
} while (0)

#if 0	/* Cause a link error for bus_space_write_multi_8 */
#define	bus_space_write_multi_8(t, h, o, a, c)				\
			!!! bus_space_write_multi_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define	bus_space_write_region_1(t, h, o, a, c) do {	\
	if ((t) == I386_BUS_SPACE_IO) {						\
		int __x __asm__("%eax");						\
		__asm __volatile("								\
			cld					;						\
		1:	lodsb					;					\
			outb %%al,%w1				;				\
			incl %1					;					\
			loop 1b"				: 					\
		    "=&a" (__x)					:				\
		    "d" ((h) + (o)), "S" ((a)), "c" ((c))	:	\
		    "%edx", "%esi", "%ecx", "memory");			\
	} else {											\
		__asm __volatile("								\
			cld					;						\
			repne					;					\
			movsb"					:					\
								:						\
		    "D" ((h) + (o)), "S" ((a)), "c" ((c))	:	\
		    "%edi", "%esi", "%ecx", "memory");			\
	}								\
} while (0)

#define	bus_space_write_region_2(t, h, o, a, c) do {	\
	if ((t) == I386_BUS_SPACE_IO) {						\
		int __x __asm__("%eax");						\
		__asm __volatile("								\
			cld					;						\
		1:	lodsw					;					\
			outw %%ax,%w1				;				\
			addl $2,%1				;					\
			loop 1b"				: 					\
		    "=&a" (__x)					:				\
		    "d" ((h) + (o)), "S" ((a)), "c" ((c))	:	\
		    "%edx", "%esi", "%ecx", "memory");			\
	} else {											\
		__asm __volatile("								\
			cld					;						\
			repne					;					\
			movsw"					:					\
								:						\
		    "D" ((h) + (o)), "S" ((a)), "c" ((c))	:	\
		    "%edi", "%esi", "%ecx", "memory");			\
	}													\
} while (0)

#define	bus_space_write_region_4(t, h, o, a, c) do {	\
	if ((t) == I386_BUS_SPACE_IO) {						\
		int __x __asm__("%eax");						\
		__asm __volatile("								\
			cld					;						\
		1:	lodsl					;					\
			outl %%eax,%w1				;				\
			addl $4,%1				;					\
			loop 1b"				: 					\
		    "=&a" (__x)					:				\
		    "d" ((h) + (o)), "S" ((a)), "c" ((c))	:	\
		    "%edx", "%esi", "%ecx", "memory");			\
	} else {											\
		__asm __volatile("								\
			cld					;						\
			repne					;					\
			movsl"					:					\
								:						\
		    "D" ((h) + (o)), "S" ((a)), "c" ((c))	:	\
		    "%edi", "%esi", "%ecx", "memory");			\
	}													\
} while (0)

#if 0	/* Cause a link error for bus_space_write_region_8 */
#define	bus_space_write_region_8					\
			!!! bus_space_write_region_8 unimplemented !!!
#endif
#endif /* _I386_BUS_SPACE_H_ */
