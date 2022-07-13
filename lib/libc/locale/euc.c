/*-
 * Copyright (c)2002 Citrus Project,
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
 */
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
 * 3. Neither the name of the University nor the names of its contributors
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
 */
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)euc.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/errno.h>

#include <rune.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "citrus_ctype.h"

typedef _Encoding_Info				_EUCEncodingInfo;
typedef _Encoding_TypeInfo 			_EUCCTypeInfo;
typedef _Encoding_State				_EUCState;

#define _FUNCNAME(m)				_EUC_citrus_ctype_##m
#define _ENCODING_MB_CUR_MAX(_ei_)	3

rune_t	_EUC_sgetrune(const char *, size_t, char const **);
int		_EUC_sputrune(rune_t, char *, size_t, char **);
int		_EUC_sgetrune_mb(_EUCEncodingInfo *, wchar_t *, const char **, size_t, _EUCState *, size_t *);
int 	_EUC_sputrune_mb(_EUCEncodingInfo *, char *, wchar_t, _EUCState *, size_t *);

int
_EUC_init(rl)
	_RuneLocale *rl;
{
	int err;

	rl->ops->ro_sgetrune = _EUC_sgetrune;
	rl->ops->ro_sputrune = _EUC_sputrune;
	rl->ops->ro_sgetrune_mb = _EUC_sgetrune_mb;
	rl->ops->ro_sputrune_mb = _EUC_sputrune_mb;

	err = _EUC_parse_variable(rl);
	if(err != 0) {
		return (err);
	}

	_CurrentRuneLocale = rl;
	return (0);
}

static int
_EUC_parse_variable(_RuneLocale *rl)
{
	_EUCEncodingInfo *ei;
	int x;
	char *v, *e;

	if (!rl->variable) {
		free(rl);
		return (EFTYPE);
	}
	v = (char*) rl->variable;

	while (*v == ' ' || *v == '\t') {
		++v;
	}
	ei = malloc(sizeof(_EUCEncodingInfo));
	if (ei == NULL) {
		free(rl);
		return (ENOMEM);
	}
	for (x = 0; x < 4; ++x) {
		ei->count[x] = (int) strtol(v, &e, 0);
		if (v == e || !(v = e)) {
			free(rl);
			free(ei);
			return (EFTYPE);
		}
		while (*v == ' ' || *v == '\t')
			++v;
		ei->bits[x] = (int) strtol(v, &e, 0);
		if (v == e || !(v = e)) {
			free(rl);
			free(ei);
			return (EFTYPE);
		}
		while (*v == ' ' || *v == '\t') {
			++v;
		}
	}
	ei->mask = (int) strtol(v, &e, 0);
	if (v == e || !(v = e)) {
		free(rl);
		free(ei);
		return (EFTYPE);
	}
	if (sizeof(_EUCEncodingInfo) <= rl->variable_len) {
		memcpy(rl->variable, ei, sizeof(_EUCEncodingInfo));
		free(ei);
	} else {
		rl->variable = &ei;
	}
	rl->variable_len = sizeof(_EUCEncodingInfo);
	return (0);
}

int
_EUC_sgetrune_mb(_EUCEncodingInfo  *ei, wchar_t *pwc, const char **s, size_t n, _EUCState *psenc, size_t *nresult)
{
	wchar_t wchar;
	const char *s0, *s1 = NULL;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(s != NULL);

	s0 = *s;

	if (s0 == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = 0; /* state independent */
		return 0;
	}

	wchar = (wchar_t) _EUC_sgetrune(s, n, nresult);

	if (pwc) {
		*pwc = wchar;
	}
	if (!wchar) {
		*nresult = 0;
	}

	return (0);
}

int
_EUC_sputrune_mb(_EUCEncodingInfo  *ei, char *s, size_t n, wchar_t wc, _EUCState *psenc, size_t *nresult)
{
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(s != NULL);

	return (_EUC_sputrune(wc, s, n, nresult));
}

#define	CEI	((_EUCEncodingInfo *)(_CurrentRuneLocale->variable))

#define	_SS2	0x008e
#define	_SS3	0x008f

static inline int
_euc_set(c)
	u_int c;
{
	c &= 0xff;

	return ((c & 0x80) ? c == _SS3 ? 3 : c == _SS2 ? 2 : 1 : 0);
}

rune_t
_EUC_sgetrune(string, n, result)
	const char *string;
	size_t n;
	char const **result;
{
	rune_t rune = 0;
	int len, set;

	if (n < 1 || (len = CEI->count[set = _euc_set(*string)]) > n) {
		if (result)
			*result = string;
		return (_INVALID_RUNE);
	}
	switch (set) {
	case 3:
	case 2:
		--len;
		++string;
		/* FALLTHROUGH */
	case 1:
	case 0:
		while (len-- > 0)
			rune = (rune << 8) | ((u_int)(*string++) & 0xff);
		break;
	}
	if (result)
		*result = string;
	return ((rune & ~CEI->mask) | CEI->bits[set]);
}

int
_EUC_sputrune(c, string, n, result)
	rune_t c;
	char *string, **result;
	size_t n;
{
	rune_t m = c & CEI->mask;
	rune_t nm = c & ~m;
	int i, len;

	if (m == CEI->bits[1]) {
CodeSet1:
		/* Codeset 1: The first byte must have 0x80 in it. */
		i = len = CEI->count[1];
		if (n >= len) {
			if (result)
				*result = string + len;
			while (i-- > 0)
				*string++ = (nm >> (i << 3)) | 0x80;
		} else
			if (result)
				*result = (char *) 0;
	} else {
		if (m == CEI->bits[0]) {
			i = len = CEI->count[0];
			if (n < len) {
				if (result)
					*result = NULL;
				return (len);
			}
		} else
			if (m == CEI->bits[2]) {
				i = len = CEI->count[2];
				if (n < len) {
					if (result)
						*result = NULL;
					return (len);
				}
				*string++ = _SS2;
				--i;
			} else
				if (m == CEI->bits[3]) {
					i = len = CEI->count[3];
					if (n < len) {
						if (result)
							*result = NULL;
						return (len);
					}
					*string++ = _SS3;
					--i;
				} else
					goto CodeSet1;	/* Bletch */
		while (i-- > 0)
			*string++ = (nm >> (i << 3)) & 0xff;
		if (result)
			*result = string;
	}
	return (len);
}
