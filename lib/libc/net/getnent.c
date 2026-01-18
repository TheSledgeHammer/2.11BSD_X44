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
static char sccsid[] = "@(#)getnetent.c	5.3 (Berkeley) 5/19/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netent.h"

#ifdef __weak_alias
__weak_alias(getnetent,_getnetent)
__weak_alias(setnetent,_setnetent)
__weak_alias(endnetent,_endnetent)
#endif

#ifdef _REENTRANT
static 	mutex_t		_ntsmutex = MUTEX_INITIALIZER;
#endif

#define	MAXALIASES	35

static char NETDB[] = _PATH_NETWORKS;

struct netent_data 	_nts_netd;
struct netent 		_nts_net;
char 			_nts_netbuf[_GETNENT_R_SIZE_MAX];

static int _nts_parsenetent(struct netent *, struct netent_data *, char *, size_t);
static int _nts_start(struct netent_data *);
static int _nts_end(struct netent_data *);
static int _nts_setnetent(struct netent_data *, int);
static int _nts_endnetent(struct netent_data *);
static int _nts_getnetent(struct netent *, struct netent_data *, char *, size_t, struct netent **);
static char *any(char *, const char *);

static int
_nts_parsenetent(np, nd, buffer, buflen)
	struct netent *np;
	struct netent_data *nd;
	char *buffer;
	size_t buflen;
{
	char *p;
	register char *cp, **q;

	if (nd->netf == NULL && (nd->netf = fopen(NETDB, "r" )) == NULL)
		return (0);

again:
	p = fgets(buffer, buflen - 1, nd->netf);
	if (p == NULL)
		return (0);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	np->n_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	np->n_net = inet_network(cp);
	np->n_addrtype = AF_INET;
	if (nd->aliases == NULL) {
		nd->maxaliases = MAXALIASES;
		nd->aliases = calloc(nd->maxaliases, sizeof(*nd->aliases));
		if (nd->aliases == NULL) {
			endnetent_r(nd);
			return (0);
		}
	}
	q = np->n_aliases = nd->aliases;
	if (p != NULL)
		cp = p;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &nd->aliases[nd->maxaliases - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (1);
}

static int
_nts_start(nd)
	struct netent_data *nd;
{
	int rval;

	if (nd->netf == NULL) {
		nd->netf = fopen(NETDB, "r" );
		rval = 0;
	} else {
		rewind(nd->netf);
		rval = 1;
	}
	return (rval);
}

static int
_nts_end(nd)
	struct netent_data *nd;
{
	if (nd->netf) {
		fclose(nd->netf);
		nd->netf = NULL;
	}
	return (1);
}

static int
_nts_setnetent(nd, stayopen)
	struct netent_data *nd;
	int stayopen;
{
	sethostent(stayopen);
	nd->stayopen |= stayopen;
	return (_nts_start(nd));
}

static int
_nts_endnetent(nd)
	struct netent_data *nd;
{
	endhostent();
	nd->stayopen = 0;
	return (_nts_end(nd));
}

static int
_nts_getnetent(np, nd, buffer, buflen, result)
	struct netent *np;
	struct netent_data *nd;
	char *buffer;
	size_t buflen;
	struct netent **result;
{
	int rval;

	rval = _nts_start(nd);
	if (rval != 1) {
		return (rval);
	}

	rval = _nts_parsenetent(np, nd, buffer, buflen);
	if (rval == 1) {
		result = &np;
	}
	return (rval);
}

int
getnetent_r(net, netd, buffer, buflen, result)
	struct netent *net;
	struct netent_data *netd;
	char *buffer;
	size_t buflen;
	struct netent **result;
{
	int rval;

	mutex_lock(&_ntsmutex);
	rval = _nts_getnetent(net, netd, buffer, buflen, result);
	mutex_unlock(&_ntsmutex);
	return (rval);
}

struct netent *
getnetent(void)
{
	struct netent *result;
	int rval;
	rval = getnetent_r(&_nts_net, &_nts_netd, _nts_netbuf, sizeof(_nts_netbuf), &result);
	return ((rval == 1) ? result : NULL);
}

void
setnetent_r(f, netd)
	int f;
	struct netent_data *netd;
{
	mutex_lock(&_ntsmutex);
	(void)_nts_setnetent(netd, f);
	mutex_unlock(&_ntsmutex);
}

void
setnetent(f)
	int f;
{
	setnetent_r(f, &_nts_netd);
}

void
endnetent_r(netd)
	struct netent_data *netd;
{
	mutex_lock(&_ntsmutex);
	(void)_nts_endnetent(netd);
	mutex_unlock(&_ntsmutex);
}

void
endnetent(void)
{
	endnetent_r(&_nts_netd);
}

static char *
any(cp, match)
	register char *cp;
	const char *match;
{
	return (strpbrk(cp, match));
}
