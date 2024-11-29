/*	$NetBSD: getenv.c,v 1.17 2003/08/07 16:43:39 agc Exp $	*/

/*
 * Copyright (c) 1987, 1993
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
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getenv.c	5.4 (Berkeley) 3/13/87";
static char sccsid[] = "@(#)getenv.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: getenv.c,v 1.17 2003/08/07 16:43:39 agc Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "local.h"
#include "reentrant.h"

extern char **environ;

char *__findenv(const char *, int);

/*
 * getenv(name) --
 *	Returns ptr to value associated with name, if any, else NULL.
 */
char *
getenv(name)
	const char *name;
{
	int	offset;
	char *result;

	_DIAGASSERT(name != NULL);

	result = __findenv(name, &offset);
	return (result);
}

/*
 * _findenv(name,offset) --
 *	Returns pointer to value associated with name, if any, else NULL.
 *	Sets offset to be the offset of the name/value combination in the
 *	environmental array, for use by setenv(3) and unsetenv(3).
 *	Explicitly removes '=' in argument name.
 *
 *	This routine *should* be a static; don't use it.
 */
char *
__findenv(name, offset)
	register const char *name;
	int	*offset;
{
	register int len;
	register const char *NP;
	register char **P, *C;

	if (name == NULL || environ == NULL) {
		return (NULL);
	}
	for (NP = name, len = 0; *NP && *NP != '='; ++NP, ++len);
	for (P = environ; (C = *P) != NULL; ++P) {
		if (!strncmp(*P, name, len)) {
			if (*(C = *P + len) == '=') {
				*offset = P - environ;
				return (++C);
			}
		}
	}
	return (NULL);
}
