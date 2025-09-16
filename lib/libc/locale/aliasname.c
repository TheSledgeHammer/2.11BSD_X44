/* $NetBSD: aliasname.c,v 1.1 2002/02/13 07:45:52 yamt Exp $ */

/*-
 * Copyright (c)2002 YAMAMOTO Takashi,
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: aliasname.c,v 1.1 2002/02/13 07:45:52 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "aliasname_local.h"

/*
 * case insensitive comparison between two C strings.
 */
int
_bcs_strcasecmp(const char * str1, const char * str2)
{
	int c1 = 1, c2 = 1;

	while (c1 && c2 && c1 == c2) {
		c1 = _bcs_toupper(*str1++);
		c2 = _bcs_toupper(*str2++);
	}

	return ((c1 == c2) ? 0 : ((c1 > c2) ? 1 : -1));
}

/*
 * case insensitive comparison between two C strings with limitation of length.
 */
int
_bcs_strncasecmp(const char * str1, const char * str2, size_t sz)
{
	int c1 = 1, c2 = 1;

	while (c1 && c2 && c1 == c2 && sz != 0) {
		c1 = _bcs_toupper(*str1++);
		c2 = _bcs_toupper(*str2++);
		sz--;
	}

	return ((c1 == c2) ? 0 : ((c1 > c2) ? 1 : -1));
}

/*
 * skip white space characters.
 */
const char *
_bcs_skip_ws(const char *p)
{
	while (*p && _bcs_isspace(*p)) {
		p++;
	}

	return (p);
}

/*
 * skip non white space characters.
 */
const char *
_bcs_skip_nonws(const char *p)
{

	while (*p && !_bcs_isspace(*p)) {
		p++;
	}

	return (p);
}

/*
 * skip white space characters with limitation of length.
 */
const char *
_bcs_skip_ws_len(const char *p, size_t *len)
{

	while (*p && *len > 0 && _bcs_isspace(*p)) {
		p++;
		(*len)--;
	}

	return (p);
}

/*
 * skip non white space characters with limitation of length.
 */
const char *
_bcs_skip_nonws_len(const char *p, size_t *len)
{

	while (*p && *len > 0 && !_bcs_isspace(*p)) {
		p++;
		(*len)--;
	}

	return (p);
}

/*
 * truncate trailing white space characters.
 */
void
_bcs_trunc_rws_len(const char *p, size_t *len)
{

	while (*len > 0 && _bcs_isspace(p[*len - 1]))
		(*len)--;
}

/*
 * destructive transliterate to lowercase.
 */
void
_bcs_convert_to_lower(char *s)
{
	while (*s) {
		*s = _bcs_tolower(*s);
		s++;
	}
}

/*
 * destructive transliterate to uppercase.
 */
void
_bcs_convert_to_upper(char *s)
{
	while (*s) {
		*s = _bcs_toupper(*s);
		s++;
	}
}

void
_bcs_ignore_case(int ignore_case, char *s)
{
	if (ignore_case) {
		_bcs_convert_to_lower(s);
	}
}

int
_bcs_is_ws(const char ch)
{
	return (ch == ' ' || ch == '\t');
}

const char *
__unaliasname(const char *dbname, const char *alias, void *buf, size_t bufsize)
{
	FILE *fp = NULL;
	const char *result = alias;
	size_t resultlen;
	size_t aliaslen;
	const char *p;
	size_t len;

	_DIAGASSERT(dbname != NULL);
	_DIAGASSERT(alias != NULL);
	_DIAGASSERT(buf != NULL);

	fp = fopen(dbname, "r");
	if (fp == NULL)
		goto quit;

	aliaslen = strlen(alias);

	while (/*CONSTCOND*/ 1) {
		p = fgetln(fp, &len);
		if (p == NULL) {
			goto quit; /* eof or error */
		}

		_DIAGASSERT(len != 0);

		/* ignore terminating NL */
		if (p[len - 1] == '\n') {
			len--;
		}

		/* ignore null line and comment */
		if (len == 0 || p[0] == '#') {
			continue;
		}

		if (aliaslen > len) {
			continue;
		}

		if (memcmp(alias, p, aliaslen)) {
			continue;
		}

		p += aliaslen;
		len -= aliaslen;

		if (len == 0 || !_bcs_is_ws(*p)) {
			continue;
		}

		/* entry was found here */
		break;

		/* NOTREACHED */
	}

	/* skip white spaces */
	do {
		p++;
		len--;
	} while (len != 0 && _bcs_is_ws(*p));

	if (len == 0) {
		goto quit;
	}

	/* count length of result */
	resultlen = 0;
	while (resultlen < len && !_bcs_is_ws(*p)) {
		resultlen++;
	}

	/* check if space is enough */
	if (bufsize < resultlen + 1) {
		goto quit;
	}

	memcpy(buf, p, resultlen);
	((char *)buf)[resultlen] = 0;
	result = buf;

quit:
	if (fp) {
		fclose(fp);
	}

	return result;
}

int
__isforcemapping(const char *name)
{
	/* don't use strcasecmp, it owes locale. */
	return name[0] == '/'
			&& (name[1] == 'F' || name[1] == 'f')
			&& (name[2] == 'O' || name[2] == 'o')
			&& (name[3] == 'R' || name[3] == 'r')
			&& (name[4] == 'C' || name[4] == 'c')
			&& (name[5] == 'E' || name[5] == 'e')
			&& name[6] == '\0';
}
