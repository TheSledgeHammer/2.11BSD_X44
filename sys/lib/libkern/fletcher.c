/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
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
 *
 */
/*
 * Fletcher Checksum:
 * Based on the following paper:
 * "Efficient Implementation of the
 * OSI Transport-Protocol
 * Checksum Algorithm
 * Using 8/16-Bit Arithmetic.
 * Alistair A.R. Cockburn,
 * IBM Research Division, Zurich Research Laboratory"
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>

#include <machine/endian.h>

#include <lib/libkern/libkern.h>

uint16_t
fletcher16(const uint8_t *buf, size_t len)
{
    uint16_t c0, c1;
    size_t first, last;
    size_t i;
    const size_t duration = 256;

    c0 = 0;
    c1 = 0;
    last = 0;
    i = 0;

	while (i < len) {
		first = last;
		last = MIN(first + duration, len);
		for (i = first; i < last; i++) {
			c0 = (c0 + buf[i]);
			c1 = (c1 + c0);
		}
		c0 = (c0 % 255);
		c1 = (c1 % 255);
	}
	if (c0 == 255) {
		c0 = 0;
	}
	if (c1 == 255) {
		c1 = 0;
	}
	return ((uint16_t)(c1 << 8) | c0);
}

uint32_t
fletcher32(const uint8_t *buf, size_t len)
{
    uint32_t c0, c1;
    size_t first, last;
    size_t i;
    const size_t duration = 65536;

    c0 = 0;
    c1 = 0;
    last = 0;
    i = 0;

	while (i < len) {
		first = last;
		last = MIN(first + duration, len);
		for (i = first; i < last; i += 2) {
			uint16_t word;
			if ((i + 1) < last) {
#if BYTE_ORDER == LITTLE_ENDIAN
				word = (uint16_t)buf[i] | ((uint16_t)buf[i + 1] << 8);
#else
				word = ((uint16_t)buf[i + 1] << 8 | (uint16_t)buf[i]);
#endif
			} else {
				word = (uint16_t)buf[i];
			}

			c0 = (c0 + word);
			c1 = (c1 + c0);
		}
		c0 = (c0 % 65535);
		c1 = (c1 % 65535);
	}
	if (c0 == 65535) {
		c0 = 0;
	}
	if (c1 == 65535) {
		c1 = 0;
	}
	return ((uint32_t)(c1 << 16) | c0);
}

#ifdef notyet
uint64_t
fletcher64(const uint8_t *buf, size_t len)
{
    uint64_t c0, c1;
    size_t first, last;
    size_t i;
    const size_t duration = 4294967296;

    c0 = 0;
    c1 = 0;
    last = 0;
    i = 0;

	while (i < len) {
		first = last;
		last = MIN(first + duration, len);
		for (i = first; i < last; i += 2) {
			uint32_t word;
			if ((i + 1) < last) {
#if BYTE_ORDER == LITTLE_ENDIAN
				word = (uint32_t)buf[i] | ((uint32_t)buf[i + 1] << 16);
#else
				word = ((uint32_t)buf[i + 1] << 16 | (uint32_t)buf[i]);
#endif
			} else {
				word = (uint32_t)buf[i];
			}
			c0 = (c0 + word);
			c1 = (c1 + c0);
		}
		c0 = (c0 % 4294967295);
		c1 = (c1 % 4294967295);
	}
	if (c0 == 4294967295) {
		c0 = 0;
	}
	if (c1 == 4294967295) {
		c1 = 0;
	}
	return ((uint64_t)(c1 << 32) | c0);
}
#endif
