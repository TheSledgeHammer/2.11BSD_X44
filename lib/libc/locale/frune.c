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

#include <limits.h>
#include <encoding.h>
#include <rune.h>
#include <stddef.h>
#include <stdio.h>

int
fgetmbrune(fp, ei, es)
	FILE *fp;
	_ENCODING_INFO  *ei;
	_ENCODING_STATE *es;
{
	wchar_t wc;
	const char buf[MB_LEN_MAX];
	size_t len, nresult;
	int ret;

	len = _ENCODING_MB_CUR_MAX(ei);
	do {
		wc = (wchar_t)getc(fp);
		if (wc == EOF) {
			if (len) {
				break;
			}
			return (EOF);
		}
		buf[len++] = wc;
		ret = sgetmbrune(ei, &wc, &buf, len, es, &nresult);
		if (ret != _INVALID_RUNE) {
			return (0);
		}
	} while (nresult == buf && len < MB_LEN_MAX);

	while (--len > 0) {
		ungetc(buf[len], fp);
	}
	return (_INVALID_RUNE);
}

int
fungetmbrune(fp, ei, es)
	FILE *fp;
	_ENCODING_INFO  *ei;
	_ENCODING_STATE *es;
{
	wchar_t wc;
	const char buf[MB_LEN_MAX];
	size_t len, nresult;
	int ret;

	len = _ENCODING_MB_CUR_MAX(ei);
	ret = sputmbrune(ei, &buf, len, &wc, es, &nresult);
	if (ret) {
		while (len-- > 0) {
			wc = (wchar_t)ungetc(buf[len], fp);
			if (wc == EOF) {
				return (EOF);
			}
		}
	}
	return (0);
}

int
fputmbrune(fp, ei, es)
	FILE *fp;
	_ENCODING_INFO  *ei;
	_ENCODING_STATE *es;
{
	wchar_t wc;
	const char buf[MB_LEN_MAX];
	size_t len, nresult;
	int i, ret;

    len = _ENCODING_MB_CUR_MAX(ei);
	ret = sputmbrune(ei, &buf, len, &wc, es, &nresult);
	if (ret) {
		  for (i = 0; i < len; ++i) {
			  wc = (wchar_t)putc(buf[i], fp);
			  if (wc == EOF) {
				  return (EOF);
			  }
		  }
	}
	return (0);
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
	} while (result == buf && len < MB_LEN_MAX);

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
