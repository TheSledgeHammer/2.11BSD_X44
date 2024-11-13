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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#ifdef __weak_alias
__weak_alias(endnetent,_endnetent)
__weak_alias(getnetent,_getnetent)
__weak_alias(setnetent,_setnetent)
#endif

#define	MAXALIASES	35

static char NETDB[] = _PATH_NETWORKS;
static FILE *netf = NULL;
static char line[BUFSIZ+1];
static struct netent net;
static char *net_aliases[MAXALIASES];
int _net_stayopen;
static char *any(char *, const char *);

void
setnetent(f)
	int f;
{
	sethostent(f);
	if (netf == NULL)
		netf = fopen(NETDB, "r" );
	else
		rewind(netf);
	_net_stayopen |= f;
}

void
endnetent(void)
{
	endhostent();
	if (netf) {
		fclose(netf);
		netf = NULL;
	}
	_net_stayopen = 0;
}

struct netent *
getnetent(void)
{
	char *p;
	register char *cp, **q;

	if (netf == NULL && (netf = fopen(NETDB, "r" )) == NULL)
		return (NULL);

again:
	p = fgets(line, sizeof(line)-1, netf);
	if (p == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	net.n_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	net.n_net = inet_network(cp);
	net.n_addrtype = AF_INET;
	q = net.n_aliases = net_aliases;
	if (p != NULL) 
		cp = p;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &net_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&net);
}

static char *
any(cp, match)
	register char *cp;
	const char *match;
{
	return (strpbrk(cp, match));
}
