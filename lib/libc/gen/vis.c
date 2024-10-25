/*	$NetBSD: vis.c,v 1.27 2004/02/26 23:01:15 enami Exp $	*/

/*-
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
static char sccsid[] = "@(#)vis.c	8.1 (Berkeley) 7/19/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <limits.h>
#include <ctype.h>
#include <vis.h>

#ifdef __weak_alias
__weak_alias(strsvis,_strsvis)
__weak_alias(strsvisx,_strsvisx)
__weak_alias(strvis,_strvis)
__weak_alias(strvisx,_strvisx)
__weak_alias(svis,_svis)
__weak_alias(vis,_vis)
#endif

#if !HAVE_VIS || !HAVE_SVIS
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#undef BELL
#define BELL '\a'

#define isoctal(c)	(((u_char)(c)) >= '0' && ((u_char)(c)) <= '7')
#define iswhite(c)	(c == ' ' || c == '\t' || c == '\n')
#define issafe(c)	(c == '\b' || c == BELL || c == '\r')
#define xtoa(c)		"0123456789abcdef"[c]

#define MAXEXTRAS	5

static char *makeextra(char *, const char *, int);
static char *makeevis(char *, int, int);
static char *makesvis(char *, int, int, int, const char *);
static char *makehvis(char *, int, int, int, const char *);

static char *
makeextra(extra, orig, flag)
	char *extra;
	const char *orig;
	int flag;
{
	const char *o;
	char *e;
	o = orig;

	while(*o++) {
		continue;
	}
	extra = malloc((size_t)((o - orig) + MAXEXTRAS));
	for (o = orig, e = extra; (*e++ = *o++) != '\0';) {
		continue;
	}
	e--;
	return (makeevis(extra, *e, flag));
}

static char *
makeevis(dst, c, flag)
	char *dst;
	int c;
	int flag;
{
	if (((u_int)c <= UCHAR_MAX && isgraph(c)) ||
	   ((flag & VIS_SP) == 0 && c == ' ') ||
	   ((flag & VIS_TAB) == 0 && c == '\t') ||
	   ((flag & VIS_NL) == 0 && c == '\n') ||
	   ((flag & VIS_SAFE) && (c == '\b' || c == '\007' || c == '\r'))) {
		*dst++ = (char)c;
		if (c == '\\' && (flag & VIS_NOSLASH) == 0) {
			*dst++ = '\\';
		}
		*dst = '\0';
		return (dst);
	}
	return (dst);
}

/*
 * This is makesvis, the central macro of vis.
 * dst:	      Pointer to the destination buffer
 * c:	      Character to encode
 * flag:      Flag word
 * nextc:     The character following 'c'
 * extra:     Pointer to the list of extra characters to be
 *	      backslash-protected.
 */
static char *
makesvis(dst, c, flag, nextc, extra)
	register char *dst;
	int c, nextc, flag;
	const char *extra;
{
	int isextra;

	isextra = strchr(extra, c) != NULL;
	if (!isextra && isascii(c)
			&& (isgraph(c) || iswhite(c) || ((flag & VIS_SAFE) && issafe(c)))) {
		*dst++ = c;
		break;
	}
	if (flag & VIS_CSTYLE) {
		switch(c) {
		case '\n':
			*dst++ = '\\';
			*dst++ = 'n';
			goto done;
		case '\r':
			*dst++ = '\\';
			*dst++ = 'r';
			goto done;
		case '\b':
			*dst++ = '\\';
			*dst++ = 'b';
			goto done;
		case '\a':
			*dst++ = '\\';
			*dst++ = 'a';
			goto done;
		case '\v':
			*dst++ = '\\';
			*dst++ = 'v';
			goto done;
		case '\t':
			*dst++ = '\\';
			*dst++ = 't';
			goto done;
		case '\f':
			*dst++ = '\\';
			*dst++ = 'f';
			goto done;
		case ' ':
			*dst++ = '\\';
			*dst++ = 's';
			goto done;
		case '\0':
			*dst++ = '\\';
			*dst++ = '0';
			if (isoctal(nextc)) {
				*dst++ = '0';
				*dst++ = '0';
			}
			goto done;
		}
	}
	if (isextra || ((c & 0177) == ' ') || (flag & VIS_OCTAL)) {
		*dst++ = '\\';
		*dst++ = ((u_char) c >> 6 & 07) + '0';
		*dst++ = ((u_char) c >> 3 & 07) + '0';
		*dst++ = ((u_char) c & 07) + '0';
		goto done;
	}
	if ((flag & VIS_NOSLASH) == 0)
		*dst++ = '\\';
	if (c & 0200) {
		c &= 0177;
		*dst++ = 'M';
	}
	if (iscntrl(c)) {
		*dst++ = '^';
		if (c == 0177)
			*dst++ = '?';
		else
			*dst++ = c + '@';
	} else {
		*dst++ = '-';
		*dst++ = c;
	}
done:
	*dst = '\0';
	return (dst);
}

/*
 * This is makehvis, the macro of vis used to HTTP style (RFC 1808)
 */
static char *
makehvis(dst, c, flag, nextc, extra)
	char *dst;
	int c, flag, nextc;
	const char *extra;
{
	if (!isascii(c) || !isalnum(c) || strchr("$-_.+!*'(),", c) != NULL) {
		*dst++ = '%';
		*dst++ = xtoa(((unsigned int)c >> 4) & 0xf);
		*dst++ = xtoa((unsigned int)c & 0xf);
	} else {
		dst = makesvis(dst, c, flag, nextc, extra);
	}
	return (dst);
}

/*
 * svis - visually encode characters, also encoding the characters
 * 	  pointed to by `extra'
 */
char *
svis(dst, c, flag, nextc, extra)
	char *dst;
	int c, flag, nextc;
	const char *extra;
{
	char *nextra;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(extra != NULL);
	dst = makeextra(nextra, extra, flag);
	if (flag & VIS_HTTPSTYLE) {
		dst = makehvis(dst, c, flag, nextc, extra);
	} else {
		dst = makesvis(dst, c, flag, nextc, extra);
	}
	return (dst);
}

/*
 * strsvis, strsvisx - visually encode characters from src into dst
 *
 *	Extra is a pointer to a \0-terminated list of characters to
 *	be encoded, too. These functions are useful e. g. to
 *	encode strings in such a way so that they are not interpreted
 *	by a shell.
 *
 *	Dst must be 4 times the size of src to account for possible
 *	expansion.  The length of dst, not including the trailing NULL,
 *	is returned.
 *
 *	Strsvisx encodes exactly len bytes from src into dst.
 *	This is useful for encoding a block of data.
 */
int
strsvis(dst, csrc, flag, extra)
	char *dst;
	const char *csrc;
	int flag;
	const char *extra;
{
	int c;
	char *start;
	char *nextra;
	const unsigned char *src = (const unsigned char *)csrc;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);
	_DIAGASSERT(extra != NULL);

	dst = makeextra(nextra, extra, flag);
	if (flag & VIS_HTTPSTYLE) {
		for (start = dst; (c = *src++) != '\0'; /* empty */) {
			dst = makehvis(dst, c, flag, *src, nextra);
		}
	} else {
		for (start = dst; (c = *src++) != '\0'; /* empty */) {
			dst = makesvis(dst, c, flag, *src, nextra);
		}
	}
	return (dst - start);
}


int
strsvisx(dst, csrc, len, flag, extra)
	char *dst;
	const char *csrc;
	size_t len;
	int flag;
	const char *extra;
{
	int c;
	char *start;
	char *nextra;
	const unsigned char *src = (const unsigned char *)csrc;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);
	_DIAGASSERT(extra != NULL);

	dst = makeextra(nextra, extra, flag);
	if (flag & VIS_HTTPSTYLE) {
		for (start = dst; len > 0; len--) {
			c = *src++;
			dst = makehvis(dst, c, flag, len ? *src : '\0', nextra);
		}
	} else {
		for (start = dst; len > 0; len--) {
			c = *src++;
			dst = makesvis(dst, c, flag, len ? *src : '\0', nextra);
		}
	}
	return (dst - start);
}
#endif

#if !HAVE_VIS
/*
 * vis - visually encode characters
 */
char *
vis(dst, c, flag, nextc)
	char *dst;
	int c, flag, nextc;
{
	char *extra;

	_DIAGASSERT(dst != NULL);

	dst = makeextra(extra, "", flag);
	if (flag & VIS_HTTPSTYLE) {
		dst = makehvis(dst, c, flag, nextc, extra);
	} else {
		dst = makesvis(dst, c, flag, nextc, extra);
	}
	return (dst);
}


/*
 * strvis, strvisx - visually encode characters from src into dst
 *
 *	Dst must be 4 times the size of src to account for possible
 *	expansion.  The length of dst, not including the trailing NULL,
 *	is returned.
 *
 *	Strvisx encodes exactly len bytes from src into dst.
 *	This is useful for encoding a block of data.
 */
int
strvis(dst, src, flag)
	char *dst;
	const char *src;
	int flag;
{
	char *extra;

	dst = makeextra(extra, "", flag);
	return (strsvis(dst, src, flag, extra));
}

int
strvisx(dst, src, len, flag)
	char *dst;
	const char *src;
	size_t len;
	int flag;
{
	char *extra;

	dst = makeextra(extra, "", flag);
	return (strsvisx(dst, src, len, flag, extra));
}
#endif
