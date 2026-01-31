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
#if 0
static char sccsid[] = "@(#)frune.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include <errno.h>
#include <limits.h>
#include <encoding.h>
#include <rune.h>
#include <stddef.h>
#include <stdio.h>

#include "reentrant.h"
#include <stdio/local.h>

wint_t
fgetmbrune(fp)
    FILE *fp;
{
	struct wchar_io_data *wcio;
	mbstate_t *st;
	wchar_t wc;
	size_t size;
	int c, len;
	char buf[MB_LEN_MAX];

	_SET_ORIENTATION(fp, 1);
	wcio = WCIO_GET(fp);
	if (wcio == 0) {
		errno = ENOMEM;
		return (WEOF );
	}

	/* if there're ungetwc'ed wchars, use them */
	if (wcio->wcio_ungetwc_inbuf) {
		wc = wcio->wcio_ungetwc_buf[--wcio->wcio_ungetwc_inbuf];
		return (wc);
	}
	st = &wcio->wcio_mbstate_in;
	len = 0;
	do {
		c = __sgetc(fp);
		if (c == EOF) {
			return (WEOF );
		}
		buf[len++] = c;
		size = mbrtowc(&wc, buf, len, st);
		if (size == (size_t) - 1) {
			fp->_flags |= __SERR;
			errno = EILSEQ;
			return (WEOF);
		} else if (size == (size_t) - 2) {
			continue;
		} else if (size == 0) {
			buf[len]++;
			len--;
			wc = L'\0';
			break;
		} else {
			buf[len] += size;
			len -= (int)size;
			break;
		}
	} while ((__srefill(fp) == 0) && (len < MB_LEN_MAX));

	if (wc == L'\0') {
		size = 1;
	}
	fp->_p = (unsigned char *)buf;
	fp->_r = len;
	return (wc);
}

wint_t
fputmbrune(wc, fp)
	wchar_t wc;
    FILE *fp;
{
	struct wchar_io_data *wcio;
	mbstate_t *st;
	size_t size, i;
	int c;
	char buf[MB_LEN_MAX];

	_SET_ORIENTATION(fp, 1);
	wcio = WCIO_GET(fp);
	if (wcio == NULL) {
		errno = ENOMEM;
		return (WEOF);
	}

	wcio->wcio_ungetwc_inbuf = 0;
	st = &wcio->wcio_mbstate_out;
	size = wcrtomb(buf, wc, st);
	if (size == (size_t) - 1) {
		errno = EILSEQ;
		return (WEOF);
	}
	for (i = 0; i < size; i++) {
		c = putc(buf[i], fp);
		if (c == EOF) {
			return (WEOF);
		}
	}
	return ((wint_t) wc);
}

wint_t
fungetmbrune(wc, fp)
	wint_t wc;
	FILE *fp;
{
	struct wchar_io_data *wcio;
	mbstate_t *st;
	size_t size;
	int c;
	char buf[MB_LEN_MAX];

	if (wc == WEOF) {
		return (WEOF );
	}

	_SET_ORIENTATION(fp, 1);
	wcio = WCIO_GET(fp);
	if (wcio == NULL) {
		errno = ENOMEM; /* XXX */
		return (WEOF);
	}

	wcio->wcio_ungetwc_inbuf = 0;
	st = &wcio->wcio_mbstate_out;
	size = wcrtomb(buf, wc, st);
	if (size == (size_t) - 1) {
		return (WEOF);
	}

	while (size-- > 0) {
		c = ungetc(buf[size], fp);
		if (c == EOF) {
			return (WEOF);
		}
	}
	return (wc);
}

long
fgetrune(fp)
	FILE *fp;
{
	rune_t  r;
	int c, len;
	char buf[MB_LEN_MAX];
	char const *result;

	len = 0;
	do {
		c = getc(fp);
		if (c == EOF) {
			if (len) {
				break;
			}
			return (EOF);
		}
		buf[len++] = c;
		r = sgetrune(buf, len, &result);
		if (r != _INVALID_RUNE) {
			return (r);
		}
	} while ((result == buf) && (len < MB_LEN_MAX));

	while (--len > 0) {
		ungetc(buf[len], fp);
	}
	return (_INVALID_RUNE);
}

int
fungetrune(r, fp)
	rune_t r;
	FILE* fp;
{
	int len, c;
	char buf[MB_LEN_MAX];

	len = sputrune(r, buf, MB_LEN_MAX, 0);
	while (len-- > 0) {
		c = ungetc(buf[len], fp);
		if (c == EOF) {
			return (EOF);
		}
	}
	return (0);
}

int
fputrune(r, fp)
	rune_t r;
	FILE *fp;
{
	int i, c, len;
	char buf[MB_LEN_MAX];

	len = sputrune(r, buf, MB_LEN_MAX, 0);
	for (i = 0; i < len; ++i) {
		c = putc(buf[i], fp);
		if (c == EOF) {
			return (EOF);
		}
	}
	return (0);
}
