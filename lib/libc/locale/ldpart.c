/*
 * Copyright (c) 2000, 2001 Alexey Zelkin <phantom@FreeBSD.org>
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
__FBSDID("$FreeBSD$");
#endif

#include "namespace.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <limits.h>
#include <stddef.h>

#include "ldpart.h"
#include "setlocale.h"

static const char nogrouping[] = { '\0' };

static int split_lines(char *, const char *);
static void set_from_buf(const char *, int, const char **);

int
__part_load_locale(const char *name,
		int *using_locale,
		char **locale_buf,
		const char *category_filename,
		int locale_buf_size_max,
		int locale_buf_size_min,
		const char **dst_localebuf)
{
	static int num_lines;
	int saverr, fd;
	char *lbuf;
	char *p;
	const char *plim;
	char filename[PATH_MAX];
	struct stat st;
	size_t namesize;
	size_t bufsize;

	/* 'name' must be already checked. */
	if (strcmp(name, _C_LOCALE) == 0 || strcmp(name, _POSIX_LOCALE) == 0) {
		*using_locale = 0;
		return (_LDP_CACHE);
	}

	/*
	 * If the locale name is the same as our cache, use the cache.
	 */
	lbuf = *locale_buf;
	if (lbuf != NULL && strcmp(name, lbuf) == 0) {
		set_from_buf(lbuf, num_lines, dst_localebuf);
		*using_locale = 1;
		return (_LDP_CACHE);
	}

	/*
	 * Slurp the locale file into the cache.
	 */
	namesize = strlen(name) + 1;

	strcpy(filename, PathLocale);
	strcat(filename, "/");
	strcat(filename, name);
	strcat(filename, "/");
	strcat(filename, category_filename);
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		return (_LDP_ERROR);
	}
	if (fstat(fd, &st) != 0)
		goto bad_locale;
	if (st.st_size <= 0) {
		errno = EFTYPE;
	}
	bufsize = namesize + st.st_size;
	*locale_buf = NULL;
	if ((lbuf = malloc(bufsize)) == NULL) {
		errno = ENOMEM;
		goto bad_locale;
	}
	(void) strcpy(lbuf, name);
	p = lbuf + namesize;
	plim = p + st.st_size;
	if (read(fd, p, (size_t) st.st_size) != st.st_size) {
		goto bad_lbuf;
	}
	/*
	 * Parse the locale file into localebuf.
	 */
	if (plim[-1] != '\n') {
		errno = EFTYPE;
		goto bad_lbuf;
	}

	num_lines = split_lines(p, plim);
	if (num_lines >= locale_buf_size_max) {
		num_lines = locale_buf_size_max;
	} else if (num_lines >= locale_buf_size_min) {
		num_lines = locale_buf_size_min;
	} else {
		errno = EFTYPE;
		goto bad_lbuf;
	}
	(void) close(fd);
	set_from_buf(lbuf, num_lines, dst_localebuf);

	/*
	 * Record the successful parse in the cache.
	 */
	if (*locale_buf != NULL) {
		free(*locale_buf);
	}
	*locale_buf = lbuf;

	*using_locale = 1;
	return (_LDP_LOADED);

bad_lbuf:
	saverr = errno;
	free(lbuf);
	errno = saverr;
bad_locale:
	saverr = errno;
	(void) close(fd);
	errno = saverr;

	return (_LDP_ERROR);
}

static int
split_lines(char *p, const char *plim)
{
	int i;

	for (i = 0; p < plim; i++) {
		p = strchr(p, '\n');
		*p++ = '\0';
	}
	return (i);
}

static void
set_from_buf(const char *p, int num_lines, const char **dst_localebuf)
{
	const char **ap;
	int i;

	for (ap = dst_localebuf, i = 0; i < num_lines; ++ap, ++i) {
		*ap = p += strlen(p) + 1;
	}
}

/*
 * Internal helper used to convert grouping sequences from string
 * representation into POSIX specified form, i.e.
 *
 * "3;3;-1" -> "\003\003\177\000"
 */

const char *
__fix_locale_grouping_str(const char *str)
{
	char *src, *dst;
	char n;

	if (str == NULL || *str == '\0') {
		return nogrouping;
	}

	for (src = (char *)&str, dst = (char *)&str; *src != '\0'; src++) {

		/* input string examples: "3;3", "3;2;-1" */
		if (*src == ';')
			continue;

		if (*src == '-' && *(src+1) == '1') {
			*dst++ = CHAR_MAX;
			src++;
			continue;
		}

		if (!isdigit((unsigned char)*src)) {
			/* broken grouping string */
			return nogrouping;
		}

		/* assume all numbers <= 99 */
		n = *src - '0';
		if (isdigit((unsigned char)*(src+1))) {
			src++;
			n *= 10;
			n += *src - '0';
		}

		*dst = n;
		/* NOTE: assume all input started with "0" as 'no grouping' */
		if (*dst == '\0')
			return (dst == (char *)&str) ? nogrouping : str;
		dst++;
	}
	*dst = '\0';
	return str;
}
