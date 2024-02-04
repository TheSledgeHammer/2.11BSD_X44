/*	$NetBSD: mktemp.c,v 1.21 2014/06/18 17:47:58 christos Exp $	*/

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
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "gettemp.h"

#if !HAVE_NBTOOL_CONFIG_H

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)mktemp.c	5.4 (Berkeley) 9/14/87";
#endif LIBC_SCCS and not lint

#include "local.h"

#if !HAVE_MKDTEMP

#ifdef __weak_alias
__weak_alias(mkdtemp,_mkdtemp)
#endif

char *
_mkdtemp(as)
	char *as;
{
	return (GETTEMP(as, NULL, 1) ? as : NULL);
}

char *
mkdtemp(as)
	char *as;
{
	_DIAGASSERT(as != NULL);

	return (GETTEMP(as, (int *)NULL, 1) ? as : (char *)NULL);
}

#endif /* !HAVE_MKDTEMP */
#if !HAVE_MKSTEMP

#ifdef __weak_alias
__weak_alias(mkstemp,_mkstemp)
#endif

char *
_mkstemp(as)
	char	*as;
{
	int	fd;

	return (GETTEMP(as, &fd, 0) ? fd : -1);
}

char *
mkstemp(as)
	char *as;
{
	int	fd;

	_DIAGASSERT(as != NULL);

	return (GETTEMP(as, &fd, 0) ? fd : -1);
}

#endif /* !HAVE_MKSTEMP */
#endif /* !HAVE_NBTOOL_CONFIG_H */

#ifdef __weak_alias
__weak_alias(mktemp,_mktemp)
#endif

char *
_mktemp(as)
	char	*as;
{
	return (GETTEMP(as, NULL, 0) ? as : NULL);
}

char *
mktemp(as)
	char	*as;
{
	_DIAGASSERT(as != NULL);

	return (GETTEMP(as, (int *)NULL, 0) ? as : (char *)NULL);
}
