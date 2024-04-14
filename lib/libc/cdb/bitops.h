/*	$NetBSD: cdbw.c,v 1.9 2023/08/08 10:34:08 riastradh Exp $	*/
/*-
 * Copyright (c) 2009, 2010, 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger and Alexander Nasonov.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

static inline int
my_fls32(uint32_t n)
{
	int v;

	if (!n)
		return 0;

	v = 32;
	if ((n & 0xFFFF0000U) == 0) {
		n <<= 16;
		v -= 16;
	}
	if ((n & 0xFF000000U) == 0) {
		n <<= 8;
		v -= 8;
	}
	if ((n & 0xF0000000U) == 0) {
		n <<= 4;
		v -= 4;
	}
	if ((n & 0xC0000000U) == 0) {
		n <<= 2;
		v -= 2;
	}
	if ((n & 0x80000000U) == 0) {
		n <<= 1;
		v -= 1;
	}
	return v;
}

static inline void
fast_divide32_prepare(uint32_t div, uint32_t * m,
    uint8_t *s1, uint8_t *s2)
{
	uint64_t mt;
	int l;

	l = my_fls32(div - 1);
	mt = (uint64_t)(0x100000000ULL * ((1ULL << l) - div));
	*m = (uint32_t)(mt / div + 1);
	*s1 = (l > 1) ? 1U : (uint8_t)l;
	*s2 = (l == 0) ? 0 : (uint8_t)(l - 1);
}

static inline uint32_t
fast_divide32(uint32_t v, uint32_t div, uint32_t m, uint8_t s1,
    uint8_t s2)
{
	uint32_t t;

	t = (uint32_t)(((uint64_t)v * m) >> 32);
	return (t + ((v - t) >> s1)) >> s2;
}

static inline uint32_t
fast_remainder32(uint32_t v, uint32_t div, uint32_t m, uint8_t s1,
    uint8_t s2)
{

	return v - div * fast_divide32(v, div, m, s1, s2);
}
