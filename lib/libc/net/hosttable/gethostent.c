/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)gethostent.c	5.3 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <ndbm.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

/*
 * Internet version.
 */
#define	MAXALIASES	20
#define	MAXADDRSIZE	(sizeof (u_long))

static FILE *hostf = NULL;
static char line[160+1];
static char hostaddr[MAXADDRSIZE];
static struct hostent host;
static char *host_aliases[MAXALIASES];
static char *host_addrs[] = {
	hostaddr,
	NULL
};

/*
 * The following is shared with gethostnamadr.c
 */
const char *_host_file = _PATH_HOSTS;
int	_host_stayopen;
DBM	*_host_db;	/* set by gethostbyname(), gethostbyaddr() */

static char *any(char *, const char *);

void
sethostent(f)
	int f;
{
	if (hostf != NULL)
		rewind(hostf);
	_host_stayopen |= f;
}

void
endhostent(void)
{
	if (hostf) {
		fclose(hostf);
		hostf = NULL;
	}
	if (_host_db) {
		dbm_close(_host_db);
		_host_db = (DBM *)NULL;
	}
	_host_stayopen = 0;
}

struct hostent *
gethostent(void)
{
	char *p;
	register char *cp, **q;

	if (hostf == NULL && (hostf = fopen(_host_file, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(line, sizeof(line)-1, hostf)) == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	host.h_addr_list = host_addrs;
	*((u_long *)host.h_addr) = inet_addr(p);
	host.h_length = sizeof (u_long);
	host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	host.h_name = cp;
	q = host.h_aliases = host_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &host_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&host);
}

void
sethostfile(file)
	const char *file;
{
	_host_file = file;
}

static char *
any(cp, match)
	register char *cp;
	const char *match;
{
    return (strpbrk(cp, match));
}
