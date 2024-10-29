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

#include <assert.h>
#include <limits.h>
#include <ctype.h>
#include <vis.h>
#include <stdlib.h>

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

#define isoctal(c)	(((u_char)(c)) >= '0' && ((u_char)(c)) <= '7')
#define iswhite(c)	(c == ' ' || c == '\t' || c == '\n')
#define issafe(c)	(c == '\b' || c == '\a' || c == '\r')
#define xtoa(c)		"0123456789abcdef"[c]
#define XTOA(c)		"0123456789ABCDEF"[c]

static const char *char_hvis = "$-_.+!*'(),";
static const char *char_mvis = "#$@[\\]^`{|}~";

static char *makevis(char *, size_t *, int, int, int, const char *);
static char *isnvis(char *, size_t *, int, int, int, const char *);
static int  istrsnvis(char *, size_t *, const char *, int, const char *);
static int  istrsnvisx(char *, size_t *, const char *, size_t, int, const char *);
static char *invis(char *, size_t *, int, int, int);
static int  istrnvis(char *, size_t *, const char *, int);
static int  istrnvisx(char *, size_t *, const char *, size_t, int);

#define MAXEXTRAS   5

#define MAKEEXTRALIST(flag, extra, orig_str)				\
do {									                    \
	const char *orig = orig_str;					        \
	const char *o = orig;						            \
	char *e;							                    \
	while (*o++) {							                \
		continue;						                    \
    }                                                       \
	extra = malloc((size_t)((o - orig) + MAXEXTRAS));		\
	if (!extra) {                                           \
        break;                                              \
    }						                                \
	for (o = orig, e = extra; (*e++ = *o++) != '\0';) {		\
		continue;						                    \
    }                                                       \
	e--;								                    \
	if (flag & VIS_SP) {                                    \
        *e++ = ' ';					                        \
    }                                                       \
	if (flag & VIS_TAB) {                                   \
        *e++ = '\t';				                        \
    }                                                       \
	if (flag & VIS_NL) {                                    \
        *e++ = '\n';					                    \
    }                                                       \
	if ((flag & VIS_NOSLASH) == 0) {                        \
        *e++ = '\\';			                            \
    }                                                       \
	*e = '\0';							                    \
} while (/*CONSTCOND*/0)

static char *
makevis(dst, dlen, c, flag, nextc, extra)
	register char *dst;
	size_t *dlen;
	int c, nextc, flag;
	const char *extra;
{
	int isextra;
	size_t odlen = dlen ? *dlen : 0;

    isextra = strchr(extra, c) != NULL;
    if (((u_int)c <= UCHAR_MAX && isgraph(c)) ||
	   ((flag & VIS_SP) == 0 && c == ' ') ||
	   ((flag & VIS_TAB) == 0 && c == '\t') ||
	   ((flag & VIS_NL) == 0 && c == '\n') ||
	   ((flag & VIS_SAFE) && (c == '\b' || c == '\007' || c == '\r'))) {
		*dst++ = c;
		if (c == '\\' && (flag & VIS_NOSLASH) == 0) {
			*dst++ = '\\';
		}
        *dst = '\0';
	    return (dst);
	}
#define HAVE(x) do { 		\
	if (dlen) { 			\
		if (*dlen < (x)) { 	\
			goto out; 		\
		} 					\
		*dlen -= (x); 		\
	} 						\
} while (/*CONSTCOND*/0)
	if (!isextra && isascii(c) && (isgraph(c) || iswhite(c) || ((flag & VIS_SAFE) && issafe(c)))) {
		HAVE(1);
		*dst++ = c;
        return (dst);
	}
	if (flag & VIS_HTTPSTYLE) {
		if (!isascii(c) || !isalnum(c) || strchr(char_hvis, c) != NULL) {
			HAVE(3);
			*dst++ = '%';
			*dst++ = xtoa(((unsigned int )c >> 4) & 0xf);
			*dst++ = xtoa((unsigned int )c & 0xf);
			goto done;
		}
	}
	if (flag & VIS_MIMESTYLE) {
		 if ((c != '\n') &&
				 /* Space at the end of the line */
			    ((isspace(c) && (nextc == '\r' || nextc == '\n')) ||
			    /* Out of range */
			    (!isspace(c) && (c < 33 || (c > 60 && c < 62) || c > 126)) ||
			    /* Specific char to be escaped */
			    strchr(char_mvis, c) != NULL)) {
			 HAVE(3);
			*dst++ = '=';
			*dst++ = XTOA(((unsigned int )c >> 4) & 0xf);
			*dst++ = XTOA((unsigned int )c & 0xf);
			goto done;
		 }
	}
	if (flag & VIS_CSTYLE) {
		HAVE(2);
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
                HAVE(2);
				*dst++ = '0';
				*dst++ = '0';
			}
			goto done;
        default:
            if (isgraph(c)) {
                *dst++ = '\\';
                *dst++ = c;
            }
            if (dlen) {
                *dlen = odlen;
            }
            goto done;
		}
	}
	if (isextra || ((c & 0177) == ' ') || (flag & VIS_OCTAL)) {
		HAVE(4);
		*dst++ = '\\';
		*dst++ = ((u_char) c >> 6 & 07) + '0';
		*dst++ = ((u_char) c >> 3 & 07) + '0';
		*dst++ = ((u_char) c & 07) + '0';
		goto done;
	}
	if ((flag & VIS_NOSLASH) == 0) {
		HAVE(1);
		*dst++ = '\\';
	}
	if (c & 0200) {
		HAVE(1);
		c &= 0177;
		*dst++ = 'M';
	}
	if (iscntrl(c)) {
		HAVE(2);
		*dst++ = '^';
		if (c == 0177) {
			*dst++ = '?';
		} else {
			*dst++ = c + '@';
		}
	} else {
		HAVE(2);
		*dst++ = '-';
		*dst++ = c;
	}
out:
    *dlen = odlen;
done:
	*dst = '\0';
	return (dst);
}


/*
 * isnvis - visually encode characters, also encoding the characters
 *	  pointed to by `extra'
 */
static char *
isnvis(dst, dlen, c, flag, nextc, extra)
	char *dst;
	size_t *dlen;
	int c, flag, nextc;
	const char *extra;
{
	char *nextra = NULL;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(extra != NULL);
    MAKEEXTRALIST(flag, nextra, extra);
	if (!nextra) {
        if (dlen && *dlen == 0) {
            return (NULL);
        }
		*dst = '\0';
        return (dst);
	}
	dst = makevis(dst, dlen, c, flag, nextc, nextra);
	free(nextra);
    if (dst == NULL || (dlen && *dlen == 0)) {
        return (NULL);
    }
    *dst = '\0';
    return (dst);
}

char *
svis(dst, c, flag, nextc, extra)
	char *dst;
	int c, flag, nextc;
	const char *extra;
{
    return (isnvis(dst, NULL, c, flag, nextc, extra));
}

char *
snvis(dst, dlen, c, flag, nextc, extra)
	char *dst;
	size_t dlen;
	int c, flag, nextc;
	const char *extra;
{
	return (isnvis(dst, &dlen, c, flag, nextc, extra));
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
static int
istrsnvis(dst, dlen, src, flag, extra)
	char *dst;
	size_t *dlen;
	const char *src;
	int flag;
	const char *extra;
{
	char *nextra = NULL;
	char *start;
	int c;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);
	_DIAGASSERT(extra != NULL);
    MAKEEXTRALIST(flag, nextra, extra);
	if (!nextra) {
		*dst = '\0';
        return (0);
	}
	for (start = dst; (c = *src++) != '\0';) {
		dst = makevis(dst, dlen, c, flag, *src++, nextra);
        if (dst == NULL) {
            return (-1);
        }
	}
	free(nextra);
	if (dlen && *dlen == 0) {
		return (-1);
	}
	*dst = '\0';
	return ((int)(dst - start));
}

int
strsvis(dst, src, flag, extra)
	char *dst;
	const char *src;
	int flag;
	const char *extra;
{
	return (istrsnvis(dst, NULL, src, flag, extra));
}

int
strsnvis(dst, dlen, src, flag, extra)
	char *dst;
	size_t dlen;
	const char *src;
	int flag;
	const char *extra;
{
	return (istrsnvis(dst, &dlen, src, flag, extra));
}

static int
istrsnvisx(dst, dlen, src, len, flag, extra)
	char *dst;
	size_t *dlen;
	const char *src;
	size_t len;
	int flag;
	const char *extra;
{
	char *nextra = NULL;
	char *start;
	int c;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);
	_DIAGASSERT(extra != NULL);
    MAKEEXTRALIST(flag, nextra, extra);
	if (!nextra) {
	    if (dlen && *dlen == 0) {
		    return (-1);
	    }
        *dst = '\0';
		return (0);
	}
	for (start = dst; len > 0; len--) {
		c = *src++;
		dst = makevis(dst, dlen, c, flag, len > 1 ? *src : '\0', nextra);
        if (dst == NULL) {
            return (-1);
        }
	}
	free(nextra);
	if (dlen && *dlen == 0) {
		return (-1);
	}
	*dst = '\0';
	return ((int)(dst - start));
}

int
strsvisx(dst, src, len, flag, extra)
	char *dst;
	const char *src;
	size_t len;
	int flag;
	const char *extra;
{
	return (istrsnvisx(dst, NULL, src, len, flag, extra));
}

int
strsnvisx(dst, dlen, src, len, flag, extra)
	char *dst;
	size_t dlen;
	const char *src;
	size_t len;
	int flag;
	const char *extra;
{
	return (istrsnvisx(dst, &dlen, src, len, flag, extra));
}
#endif

#if !HAVE_VIS
/*
 * vis - visually encode characters
 */
static char *
invis(dst, dlen, c, flag, nextc)
	char *dst;
	size_t *dlen;
	int c, flag, nextc;
{
	char *extra = NULL;

	_DIAGASSERT(dst != NULL);
    MAKEEXTRALIST(flag, extra, "");
	if (!extra) {
        if (dlen && *dlen == 0) {
            return (NULL);
        }
		*dst = '\0';
        return (dst);
	}
	dst = makevis(dst, dlen, c, flag, c, extra);
	free(extra);
	if (dst == NULL || (dlen && *dlen == 0)) {
		return (NULL);
	}
    *dst = '\0';
    return (dst);
}

char *
vis(dst, c, flag, nextc)
	char *dst;
	int c, flag, nextc;
{
	return (invis(dst, NULL, c, flag, nextc));
}

char *
nvis(dst, dlen, c, flag, nextc)
	char *dst;
	size_t dlen;
	int c, flag, nextc;
{
	return (invis(dst, &dlen, c, flag, nextc));
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
static int
istrnvis(dst, dlen, src, flag)
	char *dst;
	size_t *dlen;
	const char *src;
	int flag;
{
	char *extra = NULL;
	char *start;
	int c;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);
    MAKEEXTRALIST(flag, extra, "");
	if (!extra) {
        if (dlen && *dlen == 0) {
            return (-1);
        }
		*dst = '\0';
        return (0);
	}
	for (start = dst; (c = *src++) != '\0';) {
		dst = makevis(dst, dlen, c, flag, *src, extra);
        if (dst == NULL) {
            return (-1);
        }
	}
	free(extra);
	if ((dlen && *dlen == 0)) {
		return (-1);
	}
	*dst = '\0';
	return ((int)(dst - start));
}

int
strvis(dst, src, flag)
	char *dst;
	const char *src;
	int flag;
{
	return (istrnvis(dst, NULL, src, flag));
}

int
strnvis(dst, dlen, src, flag)
	char *dst;
	size_t dlen;
	const char *src;
	int flag;
{
	return (istrnvis(dst, &dlen, src, flag));
}

static int
istrnvisx(dst, dlen, src, len, flag)
	char *dst;
	size_t *dlen;
	const char *src;
	size_t len;
	int flag;
{
	char *extra = NULL;
	char *start;
	int c;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);
    MAKEEXTRALIST(flag, extra, "");
	if (!extra) {
        if (dlen && *dlen == 0) {
            return (-1);
        }
        *dst = '\0';
		return (0);
	}
	for (start = dst; len > 0; len--) {
		c = *src++;
		dst = makevis(dst, dlen, c, flag, len > 1 ? *src : '\0', extra);
        if (dst == NULL) {
            return (-1);
        }
	}
	free(extra);
	if (dlen && *dlen == 0) {
		return (-1);
	}
	*dst = '\0';
	return ((int)(dst - start));
}

int
strvisx(dst, src, len, flag)
	char *dst;
	const char *src;
	size_t len;
	int flag;
{
	return (istrnvisx(dst, NULL, src, len, flag));
}

int
strnvisx(dst, dlen, src, len, flag)
	char *dst;
	size_t dlen;
	const char *src;
	size_t len;
	int flag;
{
	return (istrnvisx(dst, &dlen, src, len, flag));
}
#endif
