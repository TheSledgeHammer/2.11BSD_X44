/*	$NetBSD: rand_r.c,v 1.5 2003/08/07 16:43:43 agc Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)rand.c	5.2 (Berkeley) 3/9/86";
static char sccsid[] = "from: @(#)rand.c	5.6 (Berkeley) 6/24/91";
#else
__RCSID("$NetBSD: rand_r.c,v 1.5 2003/08/07 16:43:43 agc Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

static long randx = 1;

#if defined(pdp11)
#define RAND_SEED(x) ((x) = (((x) * 1103515245 + 12345) >> 16) & RAND_MAX)
#else
#define RAND_SEED(x) ((x) = ((x) * 1103515245 + 12345) & RAND_MAX)
#endif

void
#if __STDC__
srand(unsigned int x)
#else
srand(x)
	unsigned int x;
#endif
{
	randx = x;
}

int
#if __STDC__
rand(void)
#else
rand()
#endif
{
	return (RAND_SEED(randx));
}

int
#if __STDC__
rand_r(unsigned int *seed)
#else
rand_r(x)
	unsigned int *seed;
#endif
{
	_DIAGASSERT(seed != NULL);

	return (RAND_SEED(*seed));
}
