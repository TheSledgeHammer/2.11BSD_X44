/*	$NetBSD: strtou.c,v 1.3 2019/11/28 12:33:23 roy Exp $	*/

/*-
 * Copyright (c) 2005 The DragonFly Project.  All rights reserved.
 * Copyright (c) 2003 Citrus Project,
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
 * Created by Kamil Rytarowski, based on ID:
 * NetBSD: src/common/lib/libc/stdlib/strtoul.c,v 1.3 2008/08/20 19:58:34 oster Exp
 */

#ifndef __STRTOU_H_
#define __STRTOU_H_

#if HAVE_NBTOOL_CONFIG_H
__UINT
_FUNCNAME(nptr, endptr, base, lo, hi, rstatus)
	const char *nptr;
	char **endptr;
	__UINT lo, hi;
	int base, *rstatus;

#else

#include <locale.h>
#include "setlocale.h"

__UINT
_FUNCNAME_L(nptr, endptr, base, lo, hi, rstatus, locale)
	const char *nptr;
	char **endptr;
	__UINT lo, hi;
	int base, *rstatus;
	locale_t locale;

#endif
{
	int serrno;
	__UINT im;
	char *ep;
	int rep;

	_DIAGASSERT(hi >= lo);

	_DIAGASSERT(nptr != NULL);
	/* endptr may be NULL */

	if (endptr == NULL)
		endptr = &ep;

	if (rstatus == NULL)
		rstatus = &rep;

	serrno = errno;
	errno = 0;
#if HAVE_NBTOOL_CONFIG_H
    im = __WRAPPED(nptr, endptr, base);
#else
	im = __WRAPPED_L(nptr, endptr, base, locale);
#endif
	*rstatus = errno;
	errno = serrno;

	if (*rstatus == 0) {
		/* No digits were found */
		if (nptr == *endptr)
			*rstatus = ECANCELED;
		/* There are further characters after number */
		else if (**endptr != '\0')
			*rstatus = ENOTSUP;
	}

	if (im < lo) {
		if (*rstatus == 0)
			*rstatus = ERANGE;
		return (lo);
	}
	if (im > hi) {
		if (*rstatus == 0)
			*rstatus = ERANGE;
		return (hi);
	}

	return (im);
}
/*
__UINT
FUNCNAME_L(nptr, endptr, base, lo, hi, rstatus, locale)
	const char *nptr;
	char **endptr;
	__UINT lo, hi;
	int base, *rstatus;
	locale_t locale;
{
    return (INT_FUNCNAME_L(nptr, endptr, base, lo, hi, rstatus, locale));
}

__UINT
_FUNCNAME(nptr, endptr, base, lo, hi, rstatus)
	const char *nptr;
	char **endptr;
	__UINT lo, hi;
	int base, *rstatus;
{
	return (INT_FUNCNAME(nptr, endptr, base, lo, hi, rstatus, __get_locale()));
}
*/

#endif /* __STRTOU_H_ */
