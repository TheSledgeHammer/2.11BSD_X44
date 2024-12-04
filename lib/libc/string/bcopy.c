/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)bcopy.c	1.1 (Berkeley) 1/19/87";
#endif
#endif /* LIBC_SCCS and not lint */

#include <string.h>

#if defined(_FORTIFY_SOURCE)
#undef bcopy
#endif

int __bcopy(const char *, char *, unsigned int);

/*
 * bcopy -- vax movc3 instruction
 */
int
__bcopy(src, dst, length)
	const char *src;
	char *dst;
	unsigned int length;
{
	if (length && src != dst) {
		if (dst < src) {
			do {
				*dst++ = *src++;
			} while (--length);
		} else {			/* copy backwards */
			src += length;
			dst += length;
			do {
				*--dst = *--src;
			} while (--length);
		}
	}
	return (0);
}

void
bcopy(src0, dst0, length)
	const void *src0;
	void *dst0;
	size_t length;
{
	char *dst = (char*) dst0;
	const char *src = (const char*) src0;
	(void) __bcopy(src, dst, length);
}
