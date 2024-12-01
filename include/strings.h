/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)strings.h	5.6 (Berkeley) 12/12/88
 */

#ifndef _STRINGS_H_
#define _STRINGS_H_

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#if defined(__BSD_VISIBLE)
#include <sys/null.h>
#endif

#include <sys/cdefs.h>

#include <machine/types.h>

__BEGIN_DECLS
#if defined(__BSD_VISIBLE) || \
    (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE - 0 < 700)
int	 	bcmp(const void *, const void *, size_t);
void	bcopy(const void *, void *, size_t);
void	bzero(void *, size_t);
#endif
int	 	ffs(long);
#if defined(__BSD_VISIBLE) || \
    (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE - 0 < 700)
char	*index(const char *, int);
char	*rindex(const char *, int);
#endif
int	 	strcasecmp(const char *, const char *);
int	 	strncasecmp(const char *, const char *, size_t);
__END_DECLS

#if defined(__BSD_VISIBLE)
#include <string.h>
#endif

#if _FORTIFY_SOURCE > 0
#include <ssp/strings.h>
#endif

#endif /* !defined(_STRINGS_H_) */
