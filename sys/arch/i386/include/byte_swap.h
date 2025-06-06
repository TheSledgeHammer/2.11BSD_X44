/*	$NetBSD: byte_swap.h,v 1.6 2001/11/29 02:46:55 lukem Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#ifndef _I386_BYTE_SWAP_H_
#define	_I386_BYTE_SWAP_H_

#include <sys/types.h>

#ifdef  __GNUC__

__BEGIN_DECLS
static __inline u_int64_t __byte_swap_quad_variable(u_int64_t);
static __inline u_int32_t __byte_swap_long_variable(u_int32_t);
static __inline u_int16_t __byte_swap_word_variable(u_int16_t);

static __inline u_int64_t
__byte_swap_quad_variable(u_int64_t x)
{
	__asm volatile ( "bswap %1" : "=r" (x) : "0" (x));
	return (x);
}

static __inline u_int32_t
__byte_swap_long_variable(u_int32_t x)
{
	__asm volatile ( "bswap %1" : "=r" (x) : "0" (x));
	return (x);
}

static __inline u_int16_t
__byte_swap_word_variable(u_int16_t x)
{
	__asm __volatile ("rorw $8, %w1" : "=r" (x) : "0" (x)); 
	return (x);
}

__END_DECLS

#ifdef __OPTIMIZE__

#if defined(x86_64) || defined(PMAP_PAE_COMP)

#define __byte_swap_quad_constant(x) \
	((((x) & 0xff00000000000000ull) >> 56) | \
	 (((x) & 0x00ff000000000000ull) >> 40) | \
	 (((x) & 0x0000ff0000000000ull) >> 24) | \
	 (((x) & 0x000000ff00000000ull) >>  8) | \
	 (((x) & 0x00000000ff000000ull) <<  8) | \
	 (((x) & 0x0000000000ff0000ull) << 24) | \
	 (((x) & 0x000000000000ff00ull) << 40) | \
	 (((x) & 0x00000000000000ffull) << 56))

#else

#define	__byte_swap_long_constant_high(x) \
	__byte_swap_long_constant((u_int32_t)((x) & \
			0x00000000ffffffffULL))

#define	__byte_swap_long_constant_low(x) \
	__byte_swap_long_constant((u_int32_t)(((x) >> 32) & \
			0x00000000ffffffffULL))

#define __byte_swap_quad_constant(x) \
	((__byte_swap_long_constant_high(x) << 32) | \
			__byte_swap_long_constant_low(x))

#endif

#define	__byte_swap_long_constant(x) \
	((((x) & 0xff000000) >> 24) | \
	 (((x) & 0x00ff0000) >>  8) | \
	 (((x) & 0x0000ff00) <<  8) | \
	 (((x) & 0x000000ff) << 24))

#define	__byte_swap_word_constant(x) \
	((((x) & 0xff00) >> 8) | \
	 (((x) & 0x00ff) << 8))

#define	__byte_swap_quad(x) \
	(__builtin_constant_p((x)) ? \
	 __byte_swap_quad_constant(x) : __byte_swap_quad_variable(x))

#define	__byte_swap_long(x) \
	(__builtin_constant_p((x)) ? \
	 __byte_swap_long_constant(x) : __byte_swap_long_variable(x))

#define	__byte_swap_word(x) \
	(__builtin_constant_p((x)) ? \
	 __byte_swap_word_constant(x) : __byte_swap_word_variable(x))

#else /* __OPTIMIZE__ */

#define	__byte_swap_quad(x)	__byte_swap_quad_variable(x)
#define	__byte_swap_long(x)	__byte_swap_long_variable(x)
#define	__byte_swap_word(x)	__byte_swap_word_variable(x)

#endif /* __OPTIMIZE__ */
#endif /* __GNUC__ */

#endif /* !_I386_BYTE_SWAP_H_ */
