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
static 	mutex_t			_svsmutex = MUTEX_INITIALIZER;
#endif

#define	MAXALIASES	35

static char SERVDB[] = _PATH_SERVICES;

static struct servent_data 	_svs_servd;
static struct servent 		_svs_serv;

static void _svs_setservent(struct servent_data *, int);
static void _svs_endservent(struct servent_data *);
static struct servent *_svs_getservent(struct servent *, struct servent_data *);
static char *any(char *, const char *);

static void
_svs_setservent(sd, stayopen)
	struct servent_data *sd;
	int stayopen;
{
	if (sd->servf == NULL) {
		sd->servf = fopen(SERVDB, "r");
	} else {
		rewind(sd->servf);
	}
	sd->stayopen |= stayopen;
}

static void
_svs_endservent(sd)
	struct servent_data *sd;
{
	if (sd->servf) {
		fclose(sd->servf);
		sd->servf = NULL;
	}
	if (sd->line != NULL) {
		free(sd->line);
		sd->line = NULL;
	}
	if (sd->aliases != NULL) {
		free(sd->aliases);
		sd->aliases = NULL;
		sd->maxaliases = 0;
	}
	sd->stayopen = 0;
}

static struct servent *
_svs_getservent(sp, sd)
	struct servent *sp;
	struct servent_data *sd;
{
	char *p;
	register char *cp, **q;

	if (sd->servf == NULL && (sd->servf = fopen(SERVDB, "r")) == NULL)
		return (NULL);
again:
	if (sd->line)
		free(sd->line);
	sd->line = fparseln(sd->servf, NULL, NULL, NULL, FPARSELN_UNESCALL);
	if (sd->line == NULL)
		return (NULL);
	if ((p = fgets(sd->line, sizeof(sd->line) - 1, sd->servf)) == NULL)
		return (NULL);
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
			return (NULL);
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
	return (sp);
}

void
setservent_r(f, servd)
	int f;
	struct servent_data *servd;
{
	mutex_lock(&_svsmutex);
	_svs_setservent(servd, f);
	mutex_unlock(&_svsmutex);
}

void
endservent_r(servd)
	struct servent_data *servd;
{
	mutex_lock(&_svsmutex);
	_svs_endservent(servd);
	mutex_unlock(&_svsmutex);
}

struct servent *
getservent_r(serv, servd)
	struct servent *serv;
	struct servent_data *servd;
{
	struct servent *result;

	mutex_lock(&_svsmutex);
	result = _svs_getservent(serv, servd);
	mutex_lock(&_svsmutex);
	return (result);
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

struct servent *
getservent(void)
{
	return (getservent_r(&_svs_serv, &_svs_servd));
}

static char *
any(cp, match)
	register char *cp;
	const char *match;
{
    return (strpbrk(cp, match));
}
