/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jeffrey Mogul.
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
static char sccsid[] = "@(#)swab.c	5.3 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <string.h>
#include <unistd.h>

void __swab(const char *, char *, int);

/*
 * Swab bytes
 * Jeffrey Mogul, Stanford
 */
void
__swab(from, to, n)
	register const char *from;
	register char *to;
	register int n;
{
#ifdef pdp11
	register int temp;
#else /* !pdp11 */
	register unsigned long temp;
#endif /* pdp11 */

	n >>= 1; n++;
#define STEP 				\
	(temp) = *(from)++, 	\
	*(to)++ = *(from)++, 	\
	*(to)++ = (temp)
	/* round to multiple of 8 */
	while ((--n) & 07) {
		STEP;
	}
	n >>= 3;
	while (--n >= 0) {
		STEP;
		STEP;
		STEP;
		STEP;
		STEP;
		STEP;
		STEP;
		STEP;
	}
}

void
swab(from, to, len)
    const void *from;
	void *to;
	ssize_t len;
{
	const char *fp;
	char *tp;
	int n;

	fp = (const char*) from;
	tp = (char*) to;
	n = (int) len;
	__swab(fp, tp, n);
}
