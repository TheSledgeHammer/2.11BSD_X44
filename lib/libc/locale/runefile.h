/*-
 * Copyright (c) 2005 Ruslan Ermilov
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _RUNEFILE_H_
#define _RUNEFILE_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <stdio.h>
#include <ctype.h>

typedef uint32_t 		rune_t;
typedef uint32_t 		_RuneType;

#ifndef _CACHED_RUNES
#define	_CACHED_RUNES	(1 << 8)
#endif

#ifndef _INVALID_RUNE
#define _INVALID_RUNE   ((rune_t)-3)
#endif

/* runetype */
#define	_RUNETYPE_A		0x00000100U	/* Alpha */
#define	_RUNETYPE_C		0x00000200U	/* Control */
#define	_RUNETYPE_D		0x00000400U	/* Digit */
#define	_RUNETYPE_G		0x00000800U	/* Graph */
#define	_RUNETYPE_L		0x00001000U	/* Lower */
#define	_RUNETYPE_P		0x00002000U	/* Punct */
#define	_RUNETYPE_S		0x00004000U	/* Space */
#define	_RUNETYPE_U		0x00008000U	/* Upper */
#define	_RUNETYPE_X		0x00010000U	/* X digit */
#define	_RUNETYPE_B		0x00020000U	/* Blank */
#define	_RUNETYPE_R		0x00040000U	/* Print */
#define	_RUNETYPE_I		0x00080000U	/* Ideogram */
#define	_RUNETYPE_T		0x00100000U	/* Special */
#define	_RUNETYPE_Q		0x00200000U	/* Phonogram */
#define	_RUNETYPE_SWM	0xe0000000U	/* Mask to get screen width data */
#define	_RUNETYPE_SWS	30			/* Bits to shift to get width */
#define	_RUNETYPE_SW0	0x00000000U	/* 0 width character */
#define	_RUNETYPE_SW1	0x40000000U	/* 1 width character */
#define	_RUNETYPE_SW2	0x80000000U	/* 2 width character */
#define	_RUNETYPE_SW3	0xc0000000U	/* 3 width character */

typedef struct {
	int32_t			min;
	int32_t			max;
	int32_t			map;
} _FileRuneEntry;

typedef struct {
	char			magic[8];
	char			encoding[32];

	int32_t			invalid_rune;

	_RuneType		runetype[_CACHED_RUNES];
	int32_t			maplower[_CACHED_RUNES];
	int32_t			mapupper[_CACHED_RUNES];

	uint32_t		runetype_ext_nranges;
	uint32_t		maplower_ext_nranges;
	uint32_t		mapupper_ext_nranges;

	int32_t		    variable_len;
} _FileRuneLocale;

#define	_FILE_RUNE_MAGIC_1	"RuneMag1"

#endif /* _RUNEFILE_H_ */
