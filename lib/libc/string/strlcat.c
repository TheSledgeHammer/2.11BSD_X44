/*	$NetBSD: strlcat.c,v 1.16 2003/10/27 00:12:42 lukem Exp $	*/
/*	$OpenBSD: strlcat.c,v 1.10 2003/04/12 21:56:39 millert Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND TODD C. MILLER DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL TODD C. MILLER BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: strlcat.c,v 1.16 2003/10/27 00:12:42 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef _LIBC
#include "namespace.h"
#endif
#include <sys/types.h>
#include <assert.h>
#include <string.h>

#ifdef _LIBC
# ifdef __weak_alias
__weak_alias(strlcat, _strlcat)
# endif
#endif

#if !HAVE_STRLCAT
/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
#ifdef _LIBC
_strlcat(dst, src, siz)
#else
strlcat(dst, src, siz)
#endif
	char *dst;
	const char *src;
	size_t siz;
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return (dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return (dlen + (s - src));	/* count does not include NUL */
}
#endif
