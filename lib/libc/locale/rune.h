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
 *	@(#)rune.h	8.1 (Berkeley) 6/27/93
 */

#ifndef _RUNE_H_
#define _RUNE_H_

#include <stdio.h>
#include "runetype.h"

#define	_PATH_LOCALE			"/usr/share/locale"

#define _INVALID_RUNE   		_CurrentRuneLocale->invalid_rune

#define __sgetrune      		_CurrentRuneLocale->ops->ro_sgetrune
#define __sputrune     		 	_CurrentRuneLocale->ops->ro_sputrune
#define __sgetrune_mb      		_CurrentRuneLocale->ops->ro_sgetrune_mb
#define __sputrune_mb     		_CurrentRuneLocale->ops->ro_sputrune_mb

#define sgetrune(s, n, r)       			(*__sgetrune)((s), (n), (r))
#define sputrune(c, s, n, r)    			(*__sputrune)((c), (s), (n), (r))

#define sgetrune_mb(ei, wc, s, n, es, r)    (*__sgetrune_mb)((ei), (wc), (s), (n), (es), (r))
#define sputrune_mb(ei, s, n, wc, es, r)    (*__sputrune_mb)((ei), (s), (n), (wc), (es), (r))

/*
 * Other namespace conversion.
 */
extern size_t 					__mb_len_max_runtime;
#define __MB_LEN_MAX_RUNTIME	__mb_len_max_runtime

__BEGIN_DECLS
char			*mbrune(const char *, rune_t);
char			*mbrrune(const char *, rune_t);
char			*mbmb(const char *, char *);
long			fgetrune(FILE *);
int	 			fputrune(rune_t, FILE *);
int	 			fungetrune(rune_t, FILE *);
int	 			setrunelocale(char *);
void			setinvalidrune(rune_t);

/* See comments in <machine/ansi.h> about _BSD_RUNE_T_. */
unsigned long	___runetype(rune_t);
_RuneType 		___runetype_mb(wint_t);
rune_t			___tolower(rune_t);
wint_t			___tolower_mb(wint_t);
rune_t			___toupper(rune_t);
wint_t			___toupper_mb(wint_t);
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
__istype(rune_t c, unsigned long f)
{
	return((((_RUNE_ISCACHED(c) ? ___runetype(c) : _CurrentRuneLocale->runetype[c]) & f) ? 1 : 0);
}

static __inline int
__isctype(rune_t c, unsigned long f)
{
	return((((_RUNE_ISCACHED(c) ? 0 : _DefaultRuneLocale.runetype[c]) & f) ? 1 : 0);
}

/* _ANSI_LIBRARY is defined by lib/libc/locale/isctype.c. */
#if !defined(_ANSI_LIBRARY)
static __inline rune_t
__toupper(rune_t c)
{
	return((_RUNE_ISCACHED(c) ? ___toupper(c) : _CurrentRuneLocale->mapupper[c]);
}

static __inline rune_t
__tolower(rune_t c)
{
	return((_RUNE_ISCACHED(c) ? ___tolower(c) : _CurrentRuneLocale->maplower[c]);
}

static __inline wint_t
__toupper_mb(wint_t c)
{
	return((_RUNE_ISCACHED(c) ? ___toupper_mb(c) : _CurrentRuneLocale->mapupper[c]);
}

static __inline wint_t
__tolower_mb(wint_t c)
{
	return((_RUNE_ISCACHED(c) ? ___tolower_mb(c) : _CurrentRuneLocale->maplower[c]);
}
#endif /* !_ANSI_LIBRARY */

#else /* !_USE_CTYPE_INLINE_ */

__BEGIN_DECLS
int			__istype(rune_t, unsigned long);
int			__isctype(rune_t, unsigned long);
rune_t		toupper(rune_t);
rune_t		tolower(rune_t);
wint_t		toupper_mb(wint_t);
wint_t		tolower_mb(wint_t);
__END_DECLS
#endif /* _USE_CTYPE_INLINE_ */
#endif /* _RUNE_H_ */
