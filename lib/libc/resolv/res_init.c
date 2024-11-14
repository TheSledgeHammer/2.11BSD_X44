/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)res_init.c	6.8 (Berkeley) 3/7/88";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <string.h>

#include "res_private.h"

/*
 * Resolver configuration file. Contains the address of the
 * inital name server to query and the default domain for
 * non fully qualified domain names.
 */

#ifndef	CONFFILE
#define	CONFFILE	_PATH_RESCONF
#endif

/*
 * Resolver state default settings
 */

struct __res_state _res = {
		.retrans = RES_TIMEOUT,             /* retransmition time interval */
		.retry = 4,                         /* number of times to retransmit */
		.options = RES_DEFAULT,				/* options flags */
		.nscount = 1,                       /* number of name servers */
};


void
res_close(void)
{
	res_state statp;

	statp = &_res;
	res_nclose(statp);
}

int
res_init(void)
{
	res_state statp;

	statp = &_res;
	return (res_ninit(statp));
}

int
res_mkquery(op, dname, class, type, data, datalen, newrr_in, buf, buflen)
	int op;
	const char *dname;
	int class, type;
	const u_char *data;
	int datalen;
	const u_char *newrr_in;
	u_char *buf;
	int buflen;
{
	res_state statp;

	statp = &_res;
	return (res_nmkquery(statp, op, dname, class, type, data, datalen, newrr_in, buf, buflen));
}

int
res_search(name, class, type, answer, anslen)
	char *name;
	int class, type;
	u_char *answer;
	int anslen;
{
	res_state statp;

	statp = &_res;
	return (res_nsearch(statp, name, class, type, answer, anslen));
}

int
res_send(buf, buflen, answer, anslen)
	const u_char *buf;
	int buflen;
	u_char *answer;
	int anslen;
{
	res_state statp;

	statp = &_res;
	return (res_nsend(statp, buf, buflen, answer, anslen));
}

int
res_query(name, class, type, answer, anslen)
	char *name;
	int class, type;
	u_char *answer;
	int anslen;
{
	res_state statp;

	statp = &_res;
	return (res_nquery(statp, name, class, type, answer, anslen));
}

int
res_querydomain(name, domain, class, type, answer, anslen)
	char *name, *domain;
	int class, type;
	u_char *answer;
	int anslen;
{
	res_state statp;

	statp = &_res;
	return (res_nquerydomain(statp, name, domain, class, type, answer, anslen));
}

/*
 * Set up default settings.  If the configuration file exist, the values
 * there will have precedence.  Otherwise, the server address is set to
 * INADDR_ANY and the default domain name comes from the gethostname().
 *
 * The configuration file should only be used if you want to redefine your
 * domain or run without a server on your machine.
 *
 * Return 0 if completes successfully, -1 on error
 */
int
res_ninit(statp)
	res_state statp;
{
	register FILE *fp;
	union res_sockaddr_union u[2];
    register char *cp, **pp;
    register int n;
    char buf[BUFSIZ];
	int nserv = 0;

	memset(u, 0, sizeof(u));
#ifdef USELOOPBACK
	u[nserv].sin.sin_addr = inet_makeaddr(IN_LOOPBACKNET, 1);
#else
	u[nserv].sin.sin_addr.s_addr = INADDR_ANY;
#endif
	u[nserv].sin.sin_family = AF_INET;
	u[nserv].sin.sin_port = htons(NAMESERVER_PORT);
#ifdef HAVE_SA_LEN
	u[nserv].sin.sin_len = sizeof(struct sockaddr_in);
#endif
	nserv++;
#ifdef HAS_INET6_STRUCTS
#ifdef USELOOPBACK
	u[nserv].sin6.sin6_addr = in6addr_loopback;
#else
	u[nserv].sin6.sin6_addr = in6addr_any;
#endif
	u[nserv].sin6.sin6_family = AF_INET6;
	u[nserv].sin6.sin6_port = htons(NAMESERVER_PORT);
#ifdef HAVE_SA_LEN
	u[nserv].sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif
	nserv++;
#endif

	statp->nscount = 1;
	statp->ndots = 1;
	statp->pfcode = 0;
	statp->_vcsock = -1;
	statp->_flags = 0;
	statp->defdname[0] = '\0';
	statp->u.nscount = 0;
	statp->u.ext = malloc(sizeof(*statp->u.ext));
	if (statp->u.ext != NULL) {
        memset(statp->u.ext, 0, sizeof(*statp->u.ext));
        statp->u.ext->nsaddr_list[0].sin = statp->nsaddr;
        strcpy(statp->u.ext->nsuffix, "ip6.arpa");
        strcpy(statp->u.ext->nsuffix2, "ip6.int");
	}

	if ((fp = fopen(CONFFILE, "r")) != NULL) {
		/* read the config file */
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			/* skip comments */
			if (*buf == ';' || *buf == '#') {
				continue;
			}
			/* read default domain name */
			if (!strncmp(buf, "domain", sizeof("domain") - 1)) {
				cp = buf + sizeof("domain") - 1;
				while (*cp == ' ' || *cp == '\t') {
					cp++;
				}
				if (*cp == '\0') {
					continue;
				}
				(void) strncpy(statp->defdname, cp, sizeof(statp->defdname));
				statp->defdname[sizeof(statp->defdname) - 1] = '\0';
				if ((cp = strpbrk(statp->defdname,  " \t\n")) != NULL) {
					*cp = '\0';
				}
				continue;
			}
			/* read nameservers to query */
			if (!strncmp(buf, "nameserver", sizeof("nameserver") - 1)
					&& (nserv < MAXNS)) {
				struct addrinfo hints, *ai;

				cp = buf + sizeof("nameserver") - 1;
				while (*cp == ' ' || *cp == '\t') {
					cp++;
				}
				if (*cp == '\0') {
					continue;
				}

				statp->nsaddr_list[nserv].sin_addr.s_addr = inet_addr(cp);
				if (statp->nsaddr_list[nserv].sin_addr.s_addr == (unsigned) -1) {
					statp->nsaddr_list[nserv].sin_addr.s_addr = INADDR_ANY;
				}
				statp->nsaddr_list[nserv].sin_family = AF_INET;
				statp->nsaddr_list[nserv].sin_port = htons(NAMESERVER_PORT);
				if (++nserv >= MAXNS) {
					nserv = MAXNS;
#ifdef DEBUG
					if (statp->options & RES_DEBUG) {
						printf("MAXNS reached, reading resolv.conf\n");
					}
#endif DEBUG
				}
				continue;
			}
		}
		if (nserv > 1) {
			statp->nscount = nserv;
		}
		(void) fclose(fp);
	}
	if (statp->defdname[0] == 0) {
		if (gethostname(buf, sizeof(statp->defdname)) == 0 && (cp = strchr(buf, '.'))) {
			(void) strcpy(statp->defdname, cp + 1);
		}
	}

	/* Allow user to override the local domain definition */
	if ((cp = getenv("LOCALDOMAIN")) != NULL) {
		(void) strncpy(statp->defdname, cp, sizeof(statp->defdname));
	}

	/* find components of local domain that might be searched */
	pp = statp->dnsrch;
	*pp++ = statp->defdname;
	for (cp = statp->defdname, n = 0; *cp; cp++) {
		if (*cp == '.') {
			n++;
		}
	}
	cp = statp->defdname;
	for (; n >= LOCALDOMAINPARTS && pp < statp->dnsrch + MAXDNSRCH; n--) {
		cp = index(cp, '.');
		*pp++ = ++cp;
	}
	statp->options |= RES_INIT;
	return (0);
}

res_state
res_get_state(void)
{
	res_state statp;

	statp = &_res;
	if ((statp->options & RES_INIT) == 0 && res_ninit(statp) == -1) {
		h_errno = NETDB_INTERNAL;
		return (NULL);
	}
	return (statp);
}

void
res_put_state(statp)
	res_state statp;
{

}
