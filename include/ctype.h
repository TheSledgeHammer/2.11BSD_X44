/*	ctype.h	4.2	85/09/04	*/
/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ctype.h	8.4 (Berkeley) 1/21/94
 */

#ifndef	_CTYPE_H_
#define _CTYPE_H_

#include <sys/cdefs.h>
#include <runetype.h>

#define	_CTYPE_A		0x00000100L		/* Alpha */
#define	_CTYPE_C		0x00000200L		/* Control */
#define	_CTYPE_D		0x00000400L		/* Digit */
#define	_CTYPE_G		0x00000800L		/* Graph */
#define	_CTYPE_L		0x00001000L		/* Lower */
#define	_CTYPE_P		0x00002000L		/* Punct */
#define	_CTYPE_S		0x00004000L		/* Space */
#define	_CTYPE_U		0x00008000L		/* Upper */
#define	_CTYPE_X		0x00010000L		/* X digit */
#define	_CTYPE_B		0x00020000L		/* Blank */
#define	_CTYPE_R		0x00040000L		/* Print */
#define	_CTYPE_I		0x00080000L		/* Ideogram */
#define	_CTYPE_T		0x00100000L		/* Special */
#define	_CTYPE_Q		0x00200000L		/* Phonogram */
#define	_CTYPE_SW0		0x20000000L		/* 0 width character */
#define	_CTYPE_SW1		0x40000000L		/* 1 width character */
#define	_CTYPE_SW2		0x80000000L		/* 2 width character */
#define	_CTYPE_SW3		0xc0000000L		/* 3 width character */

extern	char			_ctype_[];
#define	isalpha(c)		((_ctype_+1)[c]&(_CTYPE_U|_CTYPE_L))
#define	isupper(c)		((_ctype_+1)[c]&_CTYPE_U)
#define	islower(c)		((_ctype_+1)[c]&_CTYPE_L)
#define	isdigit(c)		((_ctype_+1)[c]&_CTYPE_N)				/* ANSI -- locale independent */
#define	isxdigit(c)		((_ctype_+1)[c]&(_CTYPE_N|_CTYPE_X)) 	/* ANSI -- locale independent */
#define	isspace(c)		((_ctype_+1)[c]&_CTYPE_S)
#define ispunct(c)		((_ctype_+1)[c]&_CTYPE_P)
#define isalnum(c)		((_ctype_+1)[c]&(_CTYPE_U|_CTYPE_L|_CTYPE_N))
#define isprint(c)		((_ctype_+1)[c]&(_CTYPE_P|_CTYPE_U|_CTYPE_L|_CTYPE_N|_CTYPE_B))
#define isgraph(c)		((_ctype_+1)[c]&(_CTYPE_P|_CTYPE_U|_CTYPE_L|_CTYPE_N))
#define iscntrl(c)		((_ctype_+1)[c]&_CTYPE_C)
#define isascii(c)		((unsigned)(c)<=0177)
#define toupper(c)		((c)-'a'+'A')
#define tolower(c)		((c)-'A'+'a')
#define toascii(c)		((c)&0177)

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
#define	isascii(c)		((c & ~0x7F) == 0)
#define toascii(c)		((c) & 0x7F)
#define	digittoint(c)	__istype((c), 0xFF)
#define	isideogram(c)	__istype((c), _CTYPE_I)
#define	isphonogram(c)	__istype((c), _CTYPE_T)
#define	isspecial(c)	__istype((c), _CTYPE_Q)
#define isblank(c)		__istype((c), _CTYPE_B)
#define	isrune(c)		__istype((c),  0xFFFFFF00L)
#define	isnumber(c)		__istype((c), _CTYPE_D)
#define	ishexnumber(c)	__istype((c), _CTYPE_X)
#endif

/* See comments in <machine/ansi.h> about _BSD_RUNE_T_. */
__BEGIN_DECLS
unsigned long	___runetype(_BSD_RUNE_T_);
_BSD_RUNE_T_	___tolower(_BSD_RUNE_T_);
_BSD_RUNE_T_	___toupper(_BSD_RUNE_T_);
__END_DECLS

/*
 * If your compiler supports prototypes and inline functions,
 * #define _USE_CTYPE_INLINE_.  Otherwise, use the C library
 * functions.
 */
#if !defined(_USE_CTYPE_CLIBRARY_) && defined(__GNUC__) || defined(__cplusplus)
#define	_USE_CTYPE_INLINE_	1
#endif

#if defined(_USE_CTYPE_INLINE_)
static __inline int
__istype(_BSD_RUNE_T_ c, unsigned long f)
{
	return((((c & _CRMASK) ? ___runetype(c) :
	    _CurrentRuneLocale->runetype[c]) & f) ? 1 : 0);
}

static __inline int
__isctype(_BSD_RUNE_T_ c, unsigned long f)
{
	return((((c & _CRMASK) ? 0 : _DefaultRuneLocale.runetype[c]) & f) ? 1 : 0);
}

/* _ANSI_LIBRARY is defined by lib/libc/locale/isctype.c. */
#if !defined(_ANSI_LIBRARY)
static __inline _BSD_RUNE_T_
toupper(_BSD_RUNE_T_ c)
{
	return((c & _CRMASK) ? ___toupper(c) : _CurrentRuneLocale->mapupper[c]);
}

static __inline _BSD_RUNE_T_
tolower(_BSD_RUNE_T_ c)
{
	return((c & _CRMASK) ? ___tolower(c) : _CurrentRuneLocale->maplower[c]);
}
#endif /* !_ANSI_LIBRARY */

#else /* !_USE_CTYPE_INLINE_ */
__BEGIN_DECLS
int				__istype (_BSD_RUNE_T_, unsigned long);
int				__isctype (_BSD_RUNE_T_, unsigned long);
_BSD_RUNE_T_	toupper (_BSD_RUNE_T_);
_BSD_RUNE_T_	tolower (_BSD_RUNE_T_);
__END_DECLS
#endif /* _USE_CTYPE_INLINE_ */
#endif
