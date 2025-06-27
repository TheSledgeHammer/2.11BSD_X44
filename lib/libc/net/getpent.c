/*
 * Copyright (c) 1983, 1993
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
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getprotoent.c	5.3 (Berkeley) 5/19/86";
static char sccsid[] = "@(#)getprotoent.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protoent.h"

#ifdef __weak_alias
__weak_alias(endprotoent_r,_endprotoent_r)
__weak_alias(getprotoent_r,_getprotoent_r)
__weak_alias(setprotoent_r,_setprotoent_r)
__weak_alias(endprotoent,_endprotoent)
__weak_alias(getprotoent,_getprotoent)
__weak_alias(setprotoent,_setprotoent)
#endif

#ifdef _REENTRANT
static 	mutex_t		_ptsmutex = MUTEX_INITIALIZER;
#endif

#define	MAXALIASES		35
static char PROTODB[] = _PATH_PROTOCOLS;

struct protoent_data 	_pts_protod;
struct protoent 	_pts_proto;
char 			_pts_protobuf[_GETPENT_R_SIZE_MAX];

static int _pts_parseprotoent(struct protoent *, struct protoent_data *, char *, size_t);
static int _pts_start(struct protoent_data *);
static int _pts_end(struct protoent_data *);
static int _pts_setprotoent(struct protoent_data *, int);
static int _pts_endprotoent(struct protoent_data *);
static int _pts_getprotoent(struct protoent *, struct protoent_data *, char *, size_t, struct protoent **);
static char *any(char *, const char *);

static int
_pts_parseprotoent(pp, pd, buffer, buflen)
	struct protoent *pp;
	struct protoent_data *pd;
	char *buffer;
	size_t buflen;
{
	char *p;
	register char *cp, **q;

	if (pd->protof == NULL && (pd->protof = fopen(PROTODB, "r")) == NULL)
		return (0);
again:
	if ((p = fgets(buffer, buflen - 1, pd->protof)) == NULL)
		return (0);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	pp->p_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	pp->p_proto = atoi(cp);
	if (pd->aliases == NULL) {
		pd->maxaliases = MAXALIASES;
		pd->aliases = calloc(pd->maxaliases, sizeof(*pd->aliases));
		if (pd->aliases == NULL) {
			endprotoent_r(pd);
			return (0);
		}
	}
	q = pp->p_aliases = pd->aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &pd->aliases[pd->maxaliases - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (1);
}

static int
_pts_start(pd)
	struct protoent_data *pd;
{
	int rval;

	if (pd->protof == NULL) {
		pd->protof = fopen(PROTODB, "r" );
		rval = 0;
	} else {
		rewind(pd->protof);
		rval = 1;
	}
	return (rval);
}

static int
_pts_end(pd)
	struct protoent_data *pd;
{
	if (pd->protof) {
		fclose(pd->protof);
		pd->protof = NULL;
	}
	if (pd->aliases) {
		free(pd->aliases);
		pd->aliases = NULL;
		pd->maxaliases = 0;
	}
	return (1);
}

static int
_pts_setprotoent(pd, stayopen)
	struct protoent_data *pd;
	int stayopen;
{
	pd->stayopen |= stayopen;
	return (_pts_start(pd));
}

static int
_pts_endprotoent(pd)
	struct protoent_data *pd;
{
	pd->stayopen = 0;
	return (_pts_end(pd));
}

static int
_pts_getprotoent(pp, pd, buffer, buflen, result)
	struct protoent *pp;
	struct protoent_data *pd;
	char *buffer;
	size_t buflen;
	struct protoent **result;
{
	int rval;

	rval = _pts_start(pd);
	if (rval != 1) {
		return (rval);
	}

	rval = _pts_parseprotoent(pp, pd, buffer, buflen);
	if (rval == 1) {
		result = &pp;
	}
	return (rval);
}

int
getprotoent_r(proto, protod, buffer, buflen, result)
	struct protoent *proto;
	struct protoent_data *protod;
	char *buffer;
	size_t buflen;
	struct protoent **result;
{
	int rval;

	mutex_lock(&_ptsmutex);
	rval = _pts_getprotoent(proto, protod, buffer, buflen, result);
	mutex_unlock(&_ptsmutex);
	return (rval);
}

struct protoent *
getprotoent(void)
{
	struct protoent *result;
	int rval;

	rval = getprotoent_r(&_pts_proto, &_pts_protod, _pts_protobuf, sizeof(_pts_protobuf), &result);
	return ((rval == 1) ? result : NULL);
}

void
setprotoent_r(f, protod)
	int f;
	struct protoent_data *protod;
{
	mutex_lock(&_ptsmutex);
	(void) _pts_setprotoent(protod, f);
	mutex_unlock(&_ptsmutex);
}

void
endprotoent_r(protod)
	struct protoent_data *protod;
{
	mutex_lock(&_ptsmutex);
	(void) _pts_endprotoent(protod);
	mutex_unlock(&_ptsmutex);
}

void
setprotoent(f)
	int f;
{
	setprotoent_r(f, &_pts_protod);
}

void
endprotoent(void)
{
	endprotoent_r(&_pts_protod);
}

static char *
any(cp, match)
	register char *cp;
	const char *match;
{
	return (strpbrk(cp, match));
}
