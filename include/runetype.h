/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
 *	@(#)runetype.h	8.1 (Berkeley) 6/2/93
 */

#ifndef	_RUNETYPE_H_
#define	_RUNETYPE_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <sys/ansi.h>
#include <machine/ansi.h>

#ifdef  _BSD_RUNE_T_
typedef _BSD_RUNE_T_	rune_t;
#undef  _BSD_RUNE_T_
#endif

#ifdef  _BSD_WCHAR_T_
typedef _BSD_WCHAR_T_	wchar_t;
#undef  _BSD_WCHAR_T_
#endif

#ifdef	_BSD_MBSTATE_T_
typedef	_BSD_MBSTATE_T_	mbstate_t;
#undef	_BSD_MBSTATE_T_
#endif

#ifdef	_BSD_WINT_T_
typedef	_BSD_WINT_T_	wint_t;
#undef	_BSD_WINT_T_
#endif

#ifdef	_BSD_WCTRANS_T_
typedef	_BSD_WCTRANS_T_	wctrans_t;
#undef	_BSD_WCTRANS_T_
#endif

#ifdef	_BSD_WCTYPE_T_
typedef	_BSD_WCTYPE_T_	wctype_t;
#undef	_BSD_WCTYPE_T_
#endif

#ifndef WEOF
#define	WEOF 			((wint_t)-1)
#endif

typedef int32_t 		_RuneType;
typedef uint64_t		_runepad_t;

#define	_CACHED_RUNES			(1 << 8)	/* Must be a power of 2 */
#define _CRMASK(c)            	((c) & (~(_CACHED_RUNES - 1)))
#define _DEFAULT_INVALID_RUNE	((rune_t)-3)

/*
 * The lower 8 bits of runetype[] contain the digit value of the rune.
 */
typedef struct {
	rune_t			min;			/* First rune of the range */
	rune_t			max;			/* Last rune (inclusive) of the range */
	rune_t			map;			/* What first maps to in maps */
	unsigned long	*types;			/* Array of types in range */
} _RuneEntry;

typedef struct {
	int				nranges;		/* Number of ranges stored */
	_RuneEntry		*ranges;		/* Pointer to the ranges */
} _RuneRange;

/*
 * expanded rune locale declaration.  local to the host.  host endian.
 */
/*
 * wctrans stuffs.
 */
typedef struct _WCTransEntry {
	char			*name;
	rune_t			*cached;
	_RuneRange		*extmap;
} _WCTransEntry;

#define _WCTRANS_INDEX_LOWER	0
#define _WCTRANS_INDEX_UPPER	1
#define _WCTRANS_NINDEXES		2

/*
 * wctype stuffs.
 */
typedef struct _WCTypeEntry {
	char			*name;
	_RuneType		mask;
} _WCTypeEntry;

#define _WCTYPE_INDEX_ALNUM		0
#define _WCTYPE_INDEX_ALPHA		1
#define _WCTYPE_INDEX_BLANK		2
#define _WCTYPE_INDEX_CNTRL		3
#define _WCTYPE_INDEX_DIGIT		4
#define _WCTYPE_INDEX_GRAPH		5
#define _WCTYPE_INDEX_LOWER		6
#define _WCTYPE_INDEX_PRINT		7
#define _WCTYPE_INDEX_PUNCT		8
#define _WCTYPE_INDEX_SPACE		9
#define _WCTYPE_INDEX_UPPER		10
#define _WCTYPE_INDEX_XDIGIT	11
#define _WCTYPE_NINDEXES		12

struct _Encoding_Info;
struct _Encoding_State;

typedef rune_t  (*sgetrune_t)(const char *, size_t, char const **);
typedef int     (*sputrune_t)(rune_t, char *, size_t, char **);
typedef int     (*sgetmbrune_t)(struct _Encoding_Info *, wchar_t *, const char **, size_t, struct _Encoding_State *, size_t *);
typedef int     (*sputmbrune_t)(struct _Encoding_Info *, char *, size_t, wchar_t, struct _Encoding_State *, size_t *);
typedef int     (*sgetcsrune_t)(struct _Encoding_Info *, wchar_t *, uint32_t, uint32_t);
typedef int     (*sputcsrune_t)(struct _Encoding_Info *, uint32_t *, uint32_t *, wchar_t);
typedef int     (*module_init_t)(struct _Encoding_Info *, const void *, size_t);
typedef void    (*module_uninit_t)(struct _Encoding_Info *);

typedef struct _RuneOps {
	/* legacy */
    sgetrune_t      ro_sgetrune;
    sputrune_t      ro_sputrune;

	/* ctype: */
    sgetmbrune_t    ro_sgetmbrune;
    sputmbrune_t    ro_sputmbrune;

	/* stdenc */
    sgetcsrune_t    ro_sgetcsrune;
    sputcsrune_t    ro_sputcsrune;

    	/* module */
    module_init_t   ro_module_init;
    module_uninit_t ro_module_uninit;
} _RuneOps;

/*
 * ctype stuffs
 */
typedef struct {
	char						magic[8];		/* Magic saying what version we are */
	char						encoding[32];	/* ASCII name of this encoding */
	rune_t						invalid_rune;

	unsigned long				runetype[_CACHED_RUNES];
	rune_t						maplower[_CACHED_RUNES];
	rune_t						mapupper[_CACHED_RUNES];

	/*
	 * The following are to deal with Runes larger than _CACHED_RUNES - 1.
	 * Their data is actually contiguous with this structure so as to make
	 * it easier to read/write from/to disk.
	 */
	_RuneRange					runetype_ext;
	_RuneRange					maplower_ext;
	_RuneRange					mapupper_ext;

	void						*variable;		/* Data which depends on the encoding */
	int							variable_len;	/* how long that data is */

	/*
	 * the following portion is generated on the fly
	 */
	_RuneOps					*ops;
	_WCTransEntry				wctrans[_WCTRANS_NINDEXES];
	_WCTypeEntry				wctype[_WCTYPE_NINDEXES];
} _RuneLocale;

#define	_RUNE_MAGIC_1			"RuneMagi"	/* Indicates version 0 of RuneLocale */

extern _RuneLocale 				_DefaultRuneLocale;
extern _RuneLocale 				*_CurrentRuneLocale;

#endif	/* !_RUNETYPE_H_ */
