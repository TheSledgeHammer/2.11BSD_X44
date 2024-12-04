/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Arthur David Olson of the National Cancer Institute.
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
/*
 *	@(#)ialloc.c	1.1 ialloc.c 3/4/87
 */

#include <sys/cdefs.h>
/*LINTLIBRARY*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef alloc_t
#define alloc_t	size_t
#endif /* !alloc_t */

#ifdef MAL
#define NULLMAL(x)	((x) == NULL || (x) == MAL)
#else /* !MAL */
#define NULLMAL(x)	((x) == NULL)
#endif /* !MAL */

#define nonzero(n)	(((n) == 0) ? 1 : (n))

char 	*icalloc(int nelem, int elsize);
char 	*icatalloc(char *old, const char *new);
char 	*icpyalloc(const char *string);
char 	*imalloc(int n);
char 	*irealloc(char * pointer, int size);
void	ifree(char *pointer);

char *
imalloc(n)
      const int n;
{
#ifdef MAL
	register char *	result;
	
	result = malloc((alloc_t)nonzero(n));
	return (result == MAL) ? NULL : result;
#else /* !MAL */
	return malloc((alloc_t)nonzero(n));
#endif /* !MAL */
}

char *
icalloc(nelem, elsize)
	int	nelem, elsize;
{
	if (nelem == 0 || elsize == 0)
		nelem = elsize = 1;
	return calloc((alloc_t)nelem, (alloc_t)elsize);
}

char *
irealloc(pointer, size)
	char * const pointer;
	const int size;
{
	if (NULLMAL(pointer))
		return imalloc(size);
	return realloc((void *)pointer, (alloc_t)nonzero(size));
}

char *
icatalloc(old, new)
	char *old;
	const char *const new;
{
	register char *result;
	register int oldsize, newsize;

	oldsize = NULLMAL(old) ? 0 : strlen(old);
	newsize = NULLMAL(new) ? 0 : strlen(new);
	
	if ((result = irealloc(old, oldsize + newsize + 1)) != NULL)
		if (!NULLMAL(new))
			(void) strcpy(result + oldsize, new);
	return result;
}

char *
icpyalloc(string)
        const char * const	string;
{
	return icatalloc((char *) NULL, string);
}

void
ifree(p)
	char *const p;
{
	if (!NULLMAL(p))
		free(p);
}
