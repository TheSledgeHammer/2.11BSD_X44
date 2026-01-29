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
#include <encoding.h>
#include <runetype.h>

#define	_PATH_LOCALE						"/usr/share/locale"

#define _INVALID_RUNE   					_CurrentRuneLocale->invalid_rune

#define __sgetrune      					_CurrentRuneLocale->ops->ro_sgetrune
#define __sputrune     		 				_CurrentRuneLocale->ops->ro_sputrune

#define __sgetmbrune      					_CurrentRuneLocale->ops->ro_sgetmbrune
#define __sputmbrune     					_CurrentRuneLocale->ops->ro_sputmbrune

#define __sgetcsrune      					_CurrentRuneLocale->ops->ro_sgetcsrune
#define __sputcsrune     					_CurrentRuneLocale->ops->ro_sputcsrune

#define __module_init						_CurrentRuneLocale->ops->ro_module_init
#define __module_uninit						_CurrentRuneLocale->ops->ro_module_uninit

#define sgetrune(s, n, r)       				(*__sgetrune)((s), (n), (r))
#define sputrune(c, s, n, r)    				(*__sputrune)((c), (s), (n), (r))

#define sgetmbrune(ei, wc, s, n, es, r)    			(*__sgetmbrune)((ei), (wc), (s), (n), (es), (r))
#define sputmbrune(ei, s, n, wc, es, r)    			(*__sputmbrune)((ei), (s), (n), (wc), (es), (r))

#define sgetcsrune(ei, wc, csid, idx) 				(*__sgetcsrune)((ei), (wc), (csid), (idx))
#define sputcsrune(ei, csid, idx, wc)  				(*__sputcsrune)((ei), (csid), (idx), (wc))

#define module_init(ei, var, lenvar)				(*__module_init)((ei), (var), (lenvar));
#define module_uninit(ei)					(*__module_uninit)((ei));

/*
 * Other namespace conversion.
 */
extern size_t 			__mb_len_max_runtime;
#define __MB_LEN_MAX_RUNTIME	__mb_len_max_runtime

__BEGIN_DECLS
char			*mbrune(const char *, rune_t);
char			*mbrrune(const char *, rune_t);
char			*mbmb(const char *, char *);
int	 			setrunelocale(char *);
void			setinvalidrune(rune_t);
long			fgetrune(FILE *);
int	 			fputrune(rune_t, FILE *);
int	 			fungetrune(rune_t, FILE *);
wint_t			fgetmbrune(FILE *);
wint_t			fungetmbrune(wint_t, FILE *);
wint_t			fputmbrune(wchar_t, FILE *);

/* emulated 4.4BSD's sgetrune & sputrune */
rune_t			emulated_sgetrune(const char *, size_t, char const **);
int				emulated_sputrune(rune_t, char *, size_t, char **);
__END_DECLS
#endif /* _RUNE_H_ */
