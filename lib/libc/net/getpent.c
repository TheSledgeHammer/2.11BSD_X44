/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getprotoent.c	5.3 (Berkeley) 5/19/86";
#endif
#endif /* LIBC_SCCS and not lint */


#include <sys/types.h>
#include <sys/socket.h>

#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	MAXALIASES	35

static char PROTODB[] = _PATH_PROTOCOLS;
static FILE *protof = NULL;
static char line[256+1];
static struct protoent proto;
static char *proto_aliases[MAXALIASES];
static char *any(char *, const char *);
int _proto_stayopen;

void
setprotoent(f)
	int f;
{
	if (protof == NULL)
		protof = fopen(PROTODB, "r" );
	else
		rewind(protof);
	_proto_stayopen |= f;
}

void
endprotoent(void)
{
	if (protof) {
		fclose(protof);
		protof = NULL;
	}
	_proto_stayopen = 0;
}

struct protoent *
getprotoent(void)
{
	char *p;
	register char *cp, **q;

	if (protof == NULL && (protof = fopen(PROTODB, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(line, sizeof(line)-1, protof)) == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	proto.p_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	proto.p_proto = atoi(cp);
	q = proto.p_aliases = proto_aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &proto_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&proto);
}

static char *
any(cp, match)
	register char *cp;
	const char *match;
{
    return (strpbrk(cp, match));
}
