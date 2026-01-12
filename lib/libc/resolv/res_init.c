/*
 * Copyright (c) 1985, 1989, 1993
 *    The Regents of the University of California.  All rights reserved.
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
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
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

#include "namespace.h"
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HAVE_MD5
#include <hash/md5.h>

#ifndef _MD5_H_
# define _MD5_H_ 1	/* make sure we do not include rsaref md5.h file */
#endif

#ifdef __weak_alias
__weak_alias(res_init,_res_init)
__weak_alias(res_mkquery,_res_mkquery)
__weak_alias(res_query,_res_query)
__weak_alias(res_search,_res_search)
__weak_alias(res_send,__res_send)
__weak_alias(res_querydomain,__res_querydomain)
#if 0
__weak_alias(hostalias,__hostalias)
#endif
#endif

#include "res_private.h"

/*
 * Resolver configuration file. Contains the address of the
 * inital name server to query and the default domain for
 * non fully qualified domain names.
 */

#ifndef	CONFFILE
#define	CONFFILE	_PATH_RESCONF
#endif

const char *res_get_nibblesuffix(res_state);
const char *res_get_nibblesuffix2(res_state);
void res_setservers(res_state, const union res_sockaddr_union *, int);
int res_getservers(res_state, union res_sockaddr_union *, int);

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

void
res_destroy(void)
{
	res_state statp;

	statp = &_res;
	res_ndestroy(statp);
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
	const char *name;
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
	const char *name;
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
	const char *name, *domain;
	int class, type;
	u_char *answer;
	int anslen;
{
	res_state statp;

	statp = &_res;
	return (res_nquerydomain(statp, name, domain, class, type, answer, anslen));
}

char *
hostalias(name)
	const char *name;
{
	res_state statp;
	static char abuf[MAXDNAME];

	statp = &_res;
	return (__UNCONST(res_hostalias(statp, name, abuf, sizeof(abuf))));
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
	register char *cp, **pp;
	register int n;
	char buf[BUFSIZ];
	int nserv = 0;
	int haveenv = 0;
	int havesearch = 0;
	union res_sockaddr_union u[2];
	int maxns = MAXNS;

	if ((statp->options & RES_INIT) != 0U) {
		res_ndestroy(statp);
	}
	statp->_rnd = malloc(16);
	res_rndinit(statp);
	statp->id = res_nrandomid(statp);

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
	} else {
		h_errno = NETDB_INTERNAL;
		maxns = 0;
	}

	res_setservers(statp, u, nserv);

#define	MATCH(line, name) \
	(!strncmp(line, name, sizeof(name) - 1) && \
	(line[sizeof(name) - 1] == ' ' || \
	 line[sizeof(name) - 1] == '\t'))

	nserv = 0;
	if ((fp = fopen(CONFFILE, "r")) != NULL) {
		/* read the config file */
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			/* skip comments */
			if (*buf == ';' || *buf == '#') {
				continue;
			}
			/* read default domain name */
			if (MATCH(buf, "domain")) {
				if (haveenv) /*%< skip if have from environ */
					continue;
				cp = buf + sizeof("domain") - 1;
				while (*cp == ' ' || *cp == '\t') {
					cp++;
				}
				if (*cp == '\0' || (*cp == '\n')) {
					continue;
				}
				(void) strncpy(statp->defdname, cp, sizeof(statp->defdname));
				statp->defdname[sizeof(statp->defdname) - 1] = '\0';
				if ((cp = strpbrk(statp->defdname, " \t\n")) != NULL) {
					*cp = '\0';
				}
				havesearch = 0;
				continue;
			}
			/* set search list */
			if (MATCH(buf, "search")) {
				if (haveenv) /*%< skip if have from environ */
					continue;
				cp = buf + sizeof("search") - 1;
				while (*cp == ' ' || *cp == '\t')
					cp++;
				if ((*cp == '\0') || (*cp == '\n'))
					continue;
				strncpy(statp->defdname, cp, sizeof(statp->defdname) - 1);
				statp->defdname[sizeof(statp->defdname) - 1] = '\0';
				if ((cp = strchr(statp->defdname, '\n')) != NULL)
					*cp = '\0';
				/*
				 * Set search list to be blank-separated strings
				 * on rest of line.
				 */
				cp = statp->defdname;
				pp = statp->dnsrch;
				*pp++ = cp;
				for (n = 0; *cp && pp < statp->dnsrch + MAXDNSRCH; cp++) {
					if (*cp == ' ' || *cp == '\t') {
						*cp = 0;
						n = 1;
					} else if (n) {
						*pp++ = cp;
						n = 0;
					}
				}
				/* null terminate last domain if there are excess */
				while (*cp != '\0' && *cp != ' ' && *cp != '\t')
					cp++;
				*cp = '\0';
				*pp++ = 0;
				havesearch = 1;
				continue;
			}
			/* read nameservers to query */
			if (MATCH(buf, "nameserver") && nserv < maxns) {
				struct addrinfo hints, *ai;
				char sbuf[NI_MAXSERV];
				const size_t minsiz = sizeof(statp->u.ext->nsaddr_list[0]);

				cp = buf + sizeof("nameserver") - 1;
				while (*cp == ' ' || *cp == '\t') {
					cp++;
				}
				cp[strcspn(cp, ";# \t\n")] = '\0';
				if ((*cp != '\0') && (*cp != '\n')) {
					memset(&hints, 0, sizeof(hints));
					hints.ai_family = PF_UNSPEC;
					hints.ai_socktype = SOCK_DGRAM; /*dummy*/
					hints.ai_flags = AI_NUMERICHOST;
					sprintf(sbuf, "%u", NAMESERVER_PORT);
					if (getaddrinfo(cp, sbuf, &hints, &ai) == 0
							&& ai->ai_addrlen <= minsiz) {
						if (statp->u.ext != NULL) {
							memcpy(&statp->u.ext->nsaddr_list[nserv],
									ai->ai_addr, ai->ai_addrlen);
						}
						if (ai->ai_addrlen
								<= sizeof(statp->nsaddr_list[nserv])) {
							memcpy(&statp->nsaddr_list[nserv], ai->ai_addr,
									ai->ai_addrlen);
						} else {
							statp->nsaddr_list[nserv].sin_family = 0;
						}
						freeaddrinfo(ai);
						nserv++;
					}
				}
				if (nserv >= maxns) {
					nserv = maxns;
#ifdef DEBUG
					if (statp->options & RES_DEBUG) {
						printf("MAXNS reached, reading resolv.conf\n");
					}
#endif
				}
				continue;
			}
		}
		if (nserv > 0) {
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
		(void) strncpy(statp->defdname, cp, sizeof(statp->defdname) - 1);
		statp->defdname[sizeof(statp->defdname) - 1] = '\0';
		haveenv++;

		cp = statp->defdname;
		pp = statp->dnsrch;
		*pp++ = cp;
		for (n = 0; *cp && pp < statp->dnsrch + MAXDNSRCH; cp++) {
			if (*cp == '\n') { /* silly backwards compat */
				break;
			} else if (*cp == ' ' || *cp == '\t') {
				*cp = 0;
				n = 1;
			} else if (n) {
				*pp++ = cp;
				n = 0;
				havesearch = 1;
			}
		}
		/* null terminate last domain if there are excess */
		while (*cp != '\0' && *cp != ' ' && *cp != '\t' && *cp != '\n') {
			cp++;
		}
		*cp = '\0';
		*pp++ = 0;
	}

	/* find components of local domain that might be searched */
	if (havesearch == 0) {
		pp = statp->dnsrch;
		*pp++ = statp->defdname;
		for (cp = statp->defdname, n = 0; *cp; cp++) {
			if (*cp == '.') {
				n++;
			}
		}
		cp = statp->defdname;
		for (; n >= LOCALDOMAINPARTS && pp < statp->dnsrch + MAXDNSRCH; n--) {
			cp = strchr(cp, '.');
			*pp++ = ++cp;
		}
	}
	statp->options |= RES_INIT;
	return (0);
}

static u_char srnd[16];

void
res_rndinit(statp)
	res_state statp;
{
	struct timeval now;
	uint32_t u32;
	uint16_t u16;
	u_char *rnd;

	rnd = statp->_rnd == NULL ? srnd : statp->_rnd;
	gettimeofday(&now, NULL);
	u32 = (uint32_t)now.tv_sec;
	memcpy(rnd, &u32, 4);
	u32 = now.tv_usec;
	memcpy(rnd + 4, &u32, 4);
	u32 += (uint32_t)now.tv_sec;
	memcpy(rnd + 8, &u32, 4);
	u16 = getpid();
	memcpy(rnd + 12, &u16, 2);
}

u_int
res_nrandomid(statp)
	res_state statp;
{
	struct timeval now;
	uint16_t u16;
	MD5_CTX ctx;
	u_char *rnd;

	rnd = statp->_rnd == NULL ? srnd : statp->_rnd;
	gettimeofday(&now, NULL);
	u16 = (uint16_t) (now.tv_sec ^ now.tv_usec);
	memcpy(rnd + 14, &u16, 2);
#ifndef HAVE_MD5
	MD5_Init(&ctx);
	MD5_Update(&ctx, rnd, 16);
	MD5_Final(rnd, &ctx);
#else
	MD5Init(&ctx);
	MD5Update(&ctx, rnd, 16);
	MD5Final(rnd, &ctx);
#endif
	memcpy(&u16, rnd + 14, 2);
	return ((u_int) u16);
}

void
res_ndestroy(statp)
	res_state statp;
{
	res_state_ext ext;

	ext = statp->u.ext;
	res_nclose(statp);
	if (ext != NULL) {
		free(ext);
		statp->u.ext = NULL;
	}
	statp->options &= ~RES_INIT;
}

const char *
res_get_nibblesuffix(statp)
	res_state statp;
{
	if (statp->u.ext) {
		return (statp->u.ext->nsuffix);
	}
	return ("ip6.arpa");
}

const char *
res_get_nibblesuffix2(statp)
	res_state statp;
{
	if (statp->u.ext) {
		return (statp->u.ext->nsuffix2);
	}
	return ("ip6.int");
}

void
res_setservers(statp, set, cnt)
	res_state statp;
	const union res_sockaddr_union *set;
	int cnt;
{
	int i, nserv;
	size_t size;

	/* close open servers */
	res_nclose(statp);

	/* cause rtt times to be forgotten */
	statp->u.nscount = 0;

	nserv = 0;
	for (i = 0; i < cnt && nserv < MAXNS; i++) {
		switch (set->sin.sin_family) {
		case AF_INET:
			size = sizeof(set->sin);
			if (statp->u.ext)
				memcpy(&statp->u.ext->nsaddr_list[nserv], &set->sin, size);
			if (size <= sizeof(statp->nsaddr_list[nserv]))
				memcpy(&statp->nsaddr_list[nserv], &set->sin, size);
			else
				statp->nsaddr_list[nserv].sin_family = 0;
			nserv++;
			break;

#ifdef HAS_INET6_STRUCTS
		case AF_INET6:
			size = sizeof(set->sin6);
			if (statp->u.ext)
				memcpy(&statp->u.ext->nsaddr_list[nserv], &set->sin6, size);
			if (size <= sizeof(statp->nsaddr_list[nserv]))
				memcpy(&statp->nsaddr_list[nserv], &set->sin6, size);
			else
				statp->nsaddr_list[nserv].sin_family = 0;
			nserv++;
			break;
#endif

		default:
			break;
		}
		set++;
	}
	statp->nscount = nserv;
}

int
res_getservers(statp, set, cnt)
	res_state statp;
	union res_sockaddr_union *set;
	int cnt;
{
	int i;
	size_t size;
	uint16_t family;

	for (i = 0; i < statp->nscount && i < cnt; i++) {
		if (statp->u.ext)
			family = statp->u.ext->nsaddr_list[i].sin.sin_family;
		else
			family = statp->nsaddr_list[i].sin_family;

		switch (family) {
		case AF_INET:
			size = sizeof(set->sin);
			if (statp->u.ext)
				memcpy(&set->sin, &statp->u.ext->nsaddr_list[i], size);
			else
				memcpy(&set->sin, &statp->nsaddr_list[i], size);
			break;

#ifdef HAS_INET6_STRUCTS
		case AF_INET6:
			size = sizeof(set->sin6);
			if (statp->u.ext)
				memcpy(&set->sin6, &statp->u.ext->nsaddr_list[i], size);
			else
				memcpy(&set->sin6, &statp->nsaddr_list[i], size);
			break;
#endif

		default:
			set->sin.sin_family = 0;
			break;
		}
		set++;
	}
	return (statp->nscount);
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
