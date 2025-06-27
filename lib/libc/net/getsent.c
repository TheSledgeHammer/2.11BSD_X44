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
static char sccsid[] = "@(#)getservent.c	5.3.1 (2.11BSD GTE) 6/27/94";
static char sccsid[] = "@(#)getservent.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "servent.h"

#ifdef __weak_alias
__weak_alias(endservent_r,_endservent_r)
__weak_alias(getservent_r,_getservent_r)
__weak_alias(setservent_r,_setservent_r)
__weak_alias(endservent,_endservent)
__weak_alias(getservent,_getservent)
__weak_alias(setservent,_setservent)
#endif

#ifdef _REENTRANT
static 	mutex_t		_svsmutex = MUTEX_INITIALIZER;
#endif

#define	MAXALIASES		35

static char SERVDB[] = _PATH_SERVICES;

struct servent_data 	_svs_servd;
struct servent 		_svs_serv;
char 			_svs_servbuf[_GETSENT_R_SIZE_MAX];

static int _svs_parseservent(struct servent *, struct servent_data *, char *, size_t);
static int _svs_start(struct servent_data *);
static int _svs_end(struct servent_data *);
static int _svs_setservent(struct servent_data *, int);
static int _svs_endservent(struct servent_data *);
static int _svs_getservent(struct servent *, struct servent_data *, char *, size_t, struct servent **);
static char *any(char *, const char *);

static int
_svs_parseservent(sp, sd, buffer, buflen)
	struct servent *sp;
	struct servent_data *sd;
	char *buffer;
	size_t buflen;
{
	char *p;
	register char *cp, **q;

	if (sd->servf == NULL && (sd->servf = fopen(SERVDB, "r")) == NULL)
		return (0);
again:
	if ((p = fgets(buffer, buflen - 1, sd->servf)) == NULL)
		return (0);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	sp->s_name = p;
	p = any(p, " \t");
	if (p == NULL)
		goto again;
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	cp = any(p, ",/");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	sp->s_port = htons((u_short) atoi(p));
	sp->s_proto = cp;
	if (sd->aliases == NULL) {
		sd->maxaliases = MAXALIASES;
		sd->aliases = calloc(sd->maxaliases, sizeof(*sd->aliases));
		if (sd->aliases == NULL) {
			endservent_r(sd);
			return (0);
		}
	}
	q = sp->s_aliases = sd->aliases;
	cp = any(cp, " \t");
	if (cp != NULL)
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &sd->aliases[sd->maxaliases - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (1);
}

static int
_svs_start(sd)
	struct servent_data *sd;
{
	int rval;

	if (sd->servf == NULL) {
		sd->servf = fopen(SERVDB, "r");
		rval = 0;
	} else {
		rewind(sd->servf);
		rval = 1;
	}
	return (rval);
}

static int
_svs_end(sd)
	struct servent_data *sd;
{
	if (sd->servf) {
		fclose(sd->servf);
		sd->servf = NULL;
	}
	if (sd->aliases) {
		free(sd->aliases);
		sd->aliases = NULL;
		sd->maxaliases = 0;
	}
	return (1);
}

static int
_svs_setservent(sd, stayopen)
	struct servent_data *sd;
	int stayopen;
{
	sd->stayopen |= stayopen;
	return (_svs_start(sd));
}

static int
_svs_endservent(sd)
	struct servent_data *sd;
{
	sd->stayopen = 0;
	return (_svs_end(sd));
}

static int
_svs_getservent(sp, sd, buffer, buflen, result)
	struct servent *sp;
	struct servent_data *sd;
	char *buffer;
	size_t buflen;
	struct servent **result;
{
	int rval;

	rval = _svs_start(sd);
	if (rval != 1) {
		return (rval);
	}

	rval = _svs_parseservent(sp, sd, buffer, buflen);
	if (rval == 1) {
		result = &sp;
	}
	return (rval);
}

int
getservent_r(serv, servd, buffer, buflen, result)
	struct servent *serv;
	struct servent_data *servd;
	char *buffer;
	size_t buflen;
	struct servent **result;
{
	int rval;

	mutex_lock(&_svsmutex);
	rval = _svs_getservent(serv, servd, buffer, buflen, result);
	mutex_unlock(&_svsmutex);
	return (rval);
}

struct servent *
getservent(void)
{
	struct servent *result;
	int rval;

	rval = getservent_r(&_svs_serv, &_svs_servd, _svs_servbuf, sizeof(_svs_servbuf), &result);
	return ((rval == 1) ? result : NULL);
}

void
setservent_r(f, servd)
	int f;
	struct servent_data *servd;
{
	mutex_lock(&_svsmutex);
	(void)_svs_setservent(servd, f);
	mutex_unlock(&_svsmutex);
}

void
endservent_r(servd)
	struct servent_data *servd;
{
	mutex_lock(&_svsmutex);
	(void)_svs_endservent(servd);
	mutex_unlock(&_svsmutex);
}

void
setservent(f)
	int f;
{
	setservent_r(f, &_svs_servd);
}

void
endservent(void)
{
	endservent_r(&_svs_servd);
}

static char *
any(cp, match)
	register char *cp;
	const char *match;
{
	return (strpbrk(cp, match));
}
