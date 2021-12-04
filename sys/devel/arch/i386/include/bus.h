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

#include <sys/cdefs.h>
#include <sys/types.h>

#include <sys/bus.h>
#include <arch/i386/include/pio.h>

#ifdef BUS_SPACE_DEBUG
#include <sys/systm.h> /* for printf() prototype */
/*
 * Macros for sanity-checking the aligned-ness of pointers passed to
 * bus space ops.  These are not strictly necessary on the x86, but
 * could lead to performance improvements, and help catch problems
 * with drivers that would creep up on other architectures.
 */
#define	__BUS_SPACE_ALIGNED_ADDRESS(p, t)								\
	((((u_long)(p)) & (sizeof(t)-1)) == 0)

#define	__BUS_SPACE_ADDRESS_SANITY(p, t, d)								\
({																		\
	if (__BUS_SPACE_ALIGNED_ADDRESS((p), t) == 0) {						\
		printf("%s 0x%lx not aligned to %d bytes %s:%d\n",				\
		    d, (u_long)(p), sizeof(t), __FILE__, __LINE__);				\
	}																	\
	(void) 0;															\
})

#define BUS_SPACE_ALIGNED_POINTER(p, t) __BUS_SPACE_ALIGNED_ADDRESS(p, t)
#else
#define	__BUS_SPACE_ADDRESS_SANITY(p,t,d)		(void) 0
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)
#endif /* BUS_SPACE_DEBUG */

/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define	bus_space_read_1(t, h, o)									\
	((t) == I386_BUS_SPACE_IO ? (inb((h) + (o))) :\
	    (*(volatile u_int8_t *)((h) + (o))))

#define	bus_space_read_2(t, h, o)									\
	 (__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr"),	\
	  ((t) == I386_BUS_SPACE_IO ? (inw((h) + (o))) :		\
	    (*(volatile u_int16_t *)((h) + (o)))))

#define	bus_space_read_4(t, h, o)									\
	 (__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr"),	\
	  ((t) == I386_BUS_SPACE_IO ? (inl((h) + (o))) :		\
	    (*(volatile u_int32_t *)((h) + (o)))))

#define bus_space_read_stream_1 bus_space_read_1
#define bus_space_read_stream_2 bus_space_read_2
#define bus_space_read_stream_4 bus_space_read_4

#if 0	/* Cause a link error for bus_space_read_8 */
#define	bus_space_read_8(t, h, o)	!!! bus_space_read_8 unimplemented !!!
#define	bus_space_read_stream_8(t, h, o)							\
		!!! bus_space_read_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define	bus_space_read_multi_1(t, h, o, ptr, cnt)					\
do {																\
	if ((t) == I386_BUS_SPACE_IO) {									\
		insb((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	movb (%2),%%al										;	\
			stosb												;	\
			loop 1b"											: 	\
		    "=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o))       	:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_multi_2(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		insw((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	movw (%2),%%ax										;	\
			stosw												;	\
			loop 1b"											:	\
		    "=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o))       	:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_multi_4(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		insl((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	movl (%2),%%eax										;	\
			stosl												;	\
			loop 1b"											:	\
		    "=D" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o))       :       \
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_read_multi_stream_1 bus_space_read_multi_1
#define bus_space_read_multi_stream_2 bus_space_read_multi_2
#define bus_space_read_multi_stream_4 bus_space_read_multi_4

#if 0	/* Cause a link error for bus_space_read_multi_8 */
#define	bus_space_read_multi_8	!!! bus_space_read_multi_8 unimplemented !!!
#define	bus_space_read_multi_stream_8								\
		!!! bus_space_read_multi_stream_8 unimplemented !!!
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

#define	bus_space_read_region_1(t, h, o, ptr, cnt)					\
do {																\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	inb %w1,%%al										;	\
			stosb												;	\
			incl %1												;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=D" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsb"												:	\
		    "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_region_2(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	inw %w1,%%ax										;	\
			stosw												;	\
			addl $2,%1											;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=D" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsw"												:	\
		    "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_region_4(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	inl %w1,%%eax										;	\
			stosl												;	\
			addl $4,%1											;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=D" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsl"												:	\
		    "=S" (dummy1), "=D" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_read_region_stream_1 bus_space_read_region_1
#define bus_space_read_region_stream_2 bus_space_read_region_2
#define bus_space_read_region_stream_4 bus_space_read_region_4

#if 0	/* Cause a link error for bus_space_read_region_8 */
#define	bus_space_read_region_8	!!! bus_space_read_region_8 unimplemented !!!
#define	bus_space_read_region_stream_8								\
		!!! bus_space_read_region_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define	bus_space_write_1(t, h, o, v)								\
do {																\
	if ((t) == I386_BUS_SPACE_IO)									\
		outb((h) + (o), (v));										\
	else															\
		((void)(*(volatile u_int8_t *)((h) + (o)) = (v)));			\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_2(t, h, o, v)								\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO)									\
		outw((h) + (o), (v));										\
	else															\
		((void)(*(volatile u_int16_t *)((h) + (o)) = (v)));			\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_4(t, h, o, v)								\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO)									\
		outl((h) + (o), (v));										\
	else															\
		((void)(*(volatile u_int32_t *)((h) + (o)) = (v)));			\
} while (/* CONSTCOND */ 0)

#define bus_space_write_stream_1 bus_space_write_1
#define bus_space_write_stream_2 bus_space_write_2
#define bus_space_write_stream_4 bus_space_write_4

#if 0	/* Cause a link error for bus_space_write_8 */
#define	bus_space_write_8	!!! bus_space_write_8 not implemented !!!
#define	bus_space_write_stream_8									\
		!!! bus_space_write_stream_8 not implemented !!!
#endif

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define	bus_space_write_multi_1(t, h, o, ptr, cnt)					\
do {																\
	if ((t) == I386_BUS_SPACE_IO) {									\
		outsb((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsb												;	\
			movb %%al,(%2)										;	\
			loop 1b"											: 	\
		    "=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));				\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_2(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		outsw((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsw												;	\
			movw %%ax,(%2)										;	\
			loop 1b"											: 	\
		    "=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x) : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));				\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_4(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		outsl((h) + (o), (ptr), (cnt));								\
	} else {														\
		void *dummy1;												\
		int dummy2;													\
		void *dummy3;												\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsl												;	\
			movl %%eax,(%2)										;	\
			loop 1b"											: 	\
		    "=S" (dummy1), "=c" (dummy2), "=r" (dummy3), "=&a" (__x)  : \
		    "0" ((ptr)), "1" ((cnt)), "2" ((h) + (o)));				\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_stream_1 bus_space_write_multi_1
#define bus_space_write_multi_stream_2 bus_space_write_multi_2
#define bus_space_write_multi_stream_4 bus_space_write_multi_4

#if 0	/* Cause a link error for bus_space_write_multi_8 */
#define	bus_space_write_multi_8(t, h, o, ptr, cnt)					\
			!!! bus_space_write_multi_8 unimplemented !!!
#define	bus_space_write_multi_stream_8(t, h, o, ptr, cnt)			\
			!!! bus_space_write_multi_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define	bus_space_write_region_1(t, h, o, ptr, cnt)					\
do {																\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsb												;	\
			outb %%al,%w1										;	\
			incl %1												;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=S" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsb"												:	\
		    "=D" (dummy1), "=S" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_region_2(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int16_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int16_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsw												;	\
			outw %%ax,%w1										;	\
			addl $2,%1											;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=S" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsw"												:	\
		    "=D" (dummy1), "=S" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_region_4(t, h, o, ptr, cnt)					\
do {																\
	__BUS_SPACE_ADDRESS_SANITY((ptr), u_int32_t, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), u_int32_t, "bus addr");	\
	if ((t) == I386_BUS_SPACE_IO) {									\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		int __x;													\
		__asm __volatile("											\
			cld													;	\
		1:	lodsl												;	\
			outl %%eax,%w1										;	\
			addl $4,%1											;	\
			loop 1b"											: 	\
		    "=&a" (__x), "=d" (dummy1), "=S" (dummy2),				\
		    "=c" (dummy3)										:	\
		    "1" ((h) + (o)), "2" ((ptr)), "3" ((cnt))			:	\
		    "memory");												\
	} else {														\
		int dummy1;													\
		void *dummy2;												\
		int dummy3;													\
		__asm __volatile("											\
			cld													;	\
			repne												;	\
			movsl"												:	\
		    "=D" (dummy1), "=S" (dummy2), "=c" (dummy3)			:	\
		    "0" ((h) + (o)), "1" ((ptr)), "2" ((cnt))			:	\
		    "memory");												\
	}																\
} while (/* CONSTCOND */ 0)

#define bus_space_write_region_stream_1 bus_space_write_region_1
#define bus_space_write_region_stream_2 bus_space_write_region_2
#define bus_space_write_region_stream_4 bus_space_write_region_4

#if 0	/* Cause a link error for bus_space_write_region_8 */
#define	bus_space_write_region_8									\
			!!! bus_space_write_region_8 unimplemented !!!
#define	bus_space_write_region_stream_8								\
			!!! bus_space_write_region_stream_8 unimplemented !!!
#endif

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

#endif /* SYS_DEVEL_ARCH_I386_INCLUDE_BUS_H_ */
