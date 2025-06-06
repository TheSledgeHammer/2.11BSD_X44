/*
 * ++Copyright++ 1985, 1988, 1993
 * -
 * Copyright (c) 1985, 1988, 1993
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
 * -
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
 * -
 * --Copyright--
 */
/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
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
 * Copyright (c) 1985, 1988 Regents of the University of California.
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
static char sccsid[] = "@(#)gethostnamadr.c	6.31.2 (2.11BSD GTE) 6/27/94";
static char sccsid[] = "@(#)gethostnamadr.c	6.47 (Berkeley) 6/18/92";
#endif
#endif /* LIBC_SCCS and not lint */

#if defined(_LIBC)
#include "namespace.h"
#endif
#include "reentrant.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <resolv.h>
#include <syslog.h>

#define MULTI_PTRS_ARE_ALIASES 1	/* XXX - experimental */

#include <resolv/res_private.h>
#include "hostent.h"

#if defined(_LIBC) && defined(__weak_alias)
__weak_alias(gethostbyaddr,_gethostbyaddr)
__weak_alias(gethostbyname,_gethostbyname)
#endif

#ifdef _REENTRANT
static 	mutex_t		_hvsmutex = MUTEX_INITIALIZER;
#endif

#define	MAXALIASES	35
#define	MAXADDRS	35

static char HOSTDB[] = _PATH_HOSTS;

static struct in_addr  host_addr;		/* IPv4 */
static struct in6_addr host6_addr;		/* IPv6 */
static char *hostaddr[MAXADDRS + 1]; 	/* h_addr_ptrs equivalent */
static char *host_addrs[4];				/* IPv4 or IPv6 */

struct hostent_data 	_hvs_hostd;
struct hostent 			_hvs_host;
char 					_hvs_hostbuf[_GETHTENT_R_SIZE_MAX];

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

typedef union {
    HEADER hdr;
    u_char buf[MAXPACKET];
} querybuf;

typedef union {
    long al;
    char ac;
} align;

extern int h_errno;

static int addalias(char **, char *, struct hostent_data *);
static int _hvs_getanswer(struct hostent *, struct hostent_data *, res_state, querybuf *, int, int, char *, size_t);
static int _hvs_gethostbyname(struct hostent *, struct hostent_data *, res_state, querybuf *, const char *, int, char *, size_t, struct hostent **);
static int _hvs_gethostbyaddr(struct hostent *, struct hostent_data *, res_state, querybuf *, const char *, int, int, char *, size_t, struct hostent **);

static int _hvs_parsehtent(struct hostent *, struct hostent_data *, char *, size_t);
static int _hvs_start(struct hostent_data *);
static int _hvs_end(struct hostent_data *);
static int _hvs_sethtent(struct hostent_data *, int);
static int _hvs_endhtent(struct hostent_data *);
static void _hvs_sethtfile(struct hostent_data *, const char *);
static int _hvs_gethtent(struct hostent *, struct hostent_data *, char *, size_t, struct hostent **);

static int getanswer_r(struct hostent *, struct hostent_data *, res_state, querybuf *, int, int, char *, size_t, struct hostent **);
static struct hostent *getanswer(querybuf *, int, int);
static int gethostbyname_internal(struct hostent *, struct hostent_data *, res_state, querybuf *, const char *, int, char *, size_t, struct hostent **);
static int gethostbyaddr_internal(struct hostent *, struct hostent_data *, res_state, querybuf *, const char *, int, int, char *, size_t, struct hostent **);

static void map_v4v6_address(const char *, char *);
static void map_v4v6_hostent(struct hostent *, char **, char *);
static void addrsort(char **, int, res_state);
static char *any(char *, const char *);

static int
addalias(ap, bp, hostd)
	char **ap, *bp;
	struct hostent_data *hostd;
{
	ptrdiff_t off;
	char **xptr;

	off = ap - hostd->aliases;
	xptr = realloc(hostd->aliases,
			(hostd->maxaliases + 10) * sizeof(*hostd->aliases));
	if (xptr == NULL) {
		free(hostd->aliases);
		errno = ENOSPC;
		return (0);
	}
	ap = xptr + off;
	hostd->aliases = xptr;
	hostd->maxaliases += 10;
	return (1);
}

static int
_hvs_getanswer(host, hostd, statp, answer, anslen, iquery, buffer, buflen)
	struct hostent *host;
	struct hostent_data *hostd;
	res_state statp;
	querybuf *answer;
	int anslen;
	int iquery;
	char *buffer;
	size_t buflen;
{
	register HEADER *hp;
	register u_char *cp;
	register int n;
	u_char *eom;
	char *bp, **ap, *ep;
	int type, class, ancount, qdcount;
	int haveanswer, getclass;
	char **hap;

	getclass = C_ANY;
	eom = answer->buf + anslen;
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	bp = buffer;
	ep = buffer + buflen;
	cp = answer->buf + sizeof(HEADER);
	if (qdcount) {
		if (iquery) {
			n = dn_expand((u_char *)answer->buf, (u_char *)eom, (u_char *)cp,
					bp, buflen);
			if (n < 0) {
				h_errno = NO_RECOVERY;
				return (0);
			}
			cp += n + QFIXEDSZ;
			host->h_name = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
		} else {
			cp += __dn_skipname(cp, eom) + QFIXEDSZ;
		}
		while (--qdcount > 0) {
			cp += __dn_skipname(cp, eom) + QFIXEDSZ;
		}
	} else if (iquery) {
		if (hp->aa) {
			h_errno = HOST_NOT_FOUND;
		} else {
			h_errno = TRY_AGAIN;
		}
		return (0);
	}
	if (hostd->aliases == NULL) {
		hostd->maxaliases = MAXALIASES;
		hostd->aliases = calloc(hostd->maxaliases, sizeof(*hostd->aliases));
		if (hostd->aliases == NULL) {
			endhtent_r(hostd);
			return (0);
		}
	}
	ap = hostd->aliases;
	*ap = NULL;
	host->h_aliases = hostd->aliases;
	hap = hostaddr;
	*hap = NULL;
	host->h_addr_list = hostaddr;
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		n = dn_expand((u_char *)answer->buf, (u_char *)eom, (u_char *)cp,
				bp, buflen);
		if (n < 0) {
			break;
		}
		cp += n;
		type = _getshort(cp);
 		cp += INT16SZ;
		class = _getshort(cp);
 		cp += INT16SZ + INT32SZ;
		n = _getshort(cp);
		cp += INT16SZ;
		if ((type == T_A || type == T_AAAA) && type == T_CNAME) {
			cp += n;
			if (ap >= &hostd->aliases[hostd->maxaliases - 1]) {
				if (addalias(ap, bp, hostd) != 1) {
					return (0);
				}
				continue;
			}
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
			continue;
		}
		if (iquery && type == T_PTR) {
			n = dn_expand((u_char *)answer->buf, (u_char *)eom,
					(u_char *)cp, bp, buflen);
			if (n < 0) {
				break;
			}
			cp += n;
#if MULTI_PTRS_ARE_ALIASES
			if (!haveanswer) {
				host->h_name = bp;
			} else {
				if (addalias(ap, bp, hostd) != 1) {
					return (0);
				}
			}
			if (n != -1) {
				n = strlen(bp) + 1;
				bp += n;
			}
#else
			host->h_name = bp;
			if (statp->options & RES_USE_INET6) {
				n = strlen(bp) + 1;
				bp += n;
				map_v4v6_hostent(host, &bp, ep);
			}
#endif
			return (1);
		}
		if (iquery && (type == T_A || type == T_AAAA)) {
			if (strcasecmp(host->h_name, bp) != 0) {
				cp += n;
				continue;
			}
			if (type == T_AAAA) {
				struct in6_addr in6;
				bcopy(cp, &in6, IN6ADDRSZ);
				if (IN6_IS_ADDR_V4MAPPED(&in6)) {
					cp += n;
					continue;
				}
			}
		}
		if (iquery || (type != T_A || type != T_AAAA)) {
			cp += n;
			continue;
		}
		if (haveanswer) {
			if (n != host->h_length) {
				cp += n;
				continue;
			}
			if (class != getclass) {
				cp += n;
				continue;
			}
		} else {
			host->h_length = n;
			getclass = class;
			host->h_addrtype = (class == C_IN) ? (AF_INET || AF_INET6) : AF_UNSPEC;
			if (!iquery) {
				host->h_name = bp;
				bp += strlen(bp) + 1;
			}
		}
		bp += sizeof(align) - ((u_int32_t)bp % sizeof(align));
		if ((bp + n) >= (buffer + buflen)) {
			break;
		}
		bcopy(cp, *hap++ = bp, n);
		bp +=n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		*ap = NULL;
		*hap = NULL;

		if (statp->nsort && haveanswer > 1 && type == T_A) {
			addrsort(hostaddr, haveanswer, statp);
		}
		if (statp->options & RES_USE_INET6) {
			map_v4v6_hostent(host, &bp, ep);
		}
		host->h_addr = hostaddr[0];
		return (1);
	} else {
		h_errno = TRY_AGAIN;
		return (0);
	}
}

static int
_hvs_gethostbyname(host, hostd, statp, buf, name, type, buffer, buflen, result)
	struct hostent *host;
	struct hostent_data *hostd;
	res_state statp;
	querybuf *buf;
	const char *name;
	int type;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	register const char *cp;
	char hbuf[MAXHOSTNAMELEN];
	int n, af;

	switch (type) {
	case AF_INET:
		host->h_addrtype = AF_INET;
		host->h_length = INADDRSZ;
		af = T_A;
		break;
	case AF_INET6:
		host->h_addrtype = AF_INET6;
		host->h_length = IN6ADDRSZ;
		af = T_AAAA;
		break;
	default:
		h_errno = NETDB_INTERNAL;
		errno = EAFNOSUPPORT;
		return (0);
	}

	/*
	 * if there aren't any dots, it could be a user-level alias.
	 * this is also done in res_nquery() since we are not the only
	 * function that looks up host names.
	 */
	if (!strchr(name, '.') && (cp = res_hostalias(statp, name, hbuf, sizeof(hbuf)))) {
		name = cp;
	}

	/*
	 * Allocate alias's if not already
	 */
	if (hostd->aliases == NULL) {
		hostd->maxaliases = MAXALIASES;
		hostd->aliases = calloc(hostd->maxaliases,
				sizeof(*hostd->aliases));
		if (hostd->aliases == NULL) {
			endhtent_r(hostd);
			return (0);
		}
	}

	if (isdigit(name[0])) {
		for (cp = name;; ++cp) {
			if (!*cp) {
				if (*--cp == '.') {
					break;
				}
				/*
				 * All-numeric, no dot at the end.
				 * Fake up a hostent as if we'd actually
				 * done a lookup.
				 */
				if (inet_pton(type, name, host_addrs) <= 0) {
					h_errno = HOST_NOT_FOUND;
					return (0);
				}

				host->h_name = __UNCONST(name);
				host->h_aliases = hostd->aliases;
				hostd->aliases[0] = NULL;
				switch (type) {
				case AF_INET:
					hostaddr[0] = (char *)&host_addr;
					hostaddr[1] = (char *)NULL;
					break;
				case AF_INET6:
					hostaddr[0] = (char *)&host6_addr;
					hostaddr[1] = (char *)NULL;
					break;
				}
				host->h_addr_list = hostaddr;
				host->h_addr= hostaddr[0];
				if (statp->options & RES_USE_INET6) {
					map_v4v6_hostent(host, &buffer, (buffer + buflen));
				}
				h_errno = NETDB_SUCCESS;
				return (1);
			}
			if (!isdigit(*cp) && *cp != '.') {
				break;
			}
		}
	}
	if ((isxdigit((u_char) name[0]) && strchr(name, ':') != NULL)
			|| name[0] == ':') {
		for (cp = name;; ++cp) {
			if (!*cp) {
				if (*--cp == '.') {
					break;
				}
				/*
				 * All-IPv6-legal, no dot at the end.
				 * Fake up a hostent as if we'd actually
				 * done a lookup.
				 */
				if (inet_pton(type, name, host_addrs) <= 0) {
					h_errno = HOST_NOT_FOUND;
					return (0);
				}

				host->h_name = __UNCONST(name);
				host->h_aliases = hostd->aliases;
				hostd->aliases[0] = NULL;
				switch (type) {
				case AF_INET:
					hostaddr[0] = (char *)&host_addr;
					hostaddr[1] = (char *)NULL;
					break;
				case AF_INET6:
					hostaddr[0] = (char *)&host6_addr;
					hostaddr[1] = (char *)NULL;
					break;
				}
				host->h_addr_list = hostaddr;
				host->h_addr= hostaddr[0];
				h_errno = NETDB_SUCCESS;
				return (1);
			}
			if (!isxdigit((u_char) *cp) && *cp != ':' && *cp != '.') {
				break;
			}
		}
	}
	n = res_nsearch(statp, __UNCONST(name), C_IN, af, buf->buf, sizeof(buf));
	if (n < 0) {
		if (statp->options & RES_DEBUG) {
			printf("res_search failed\n");
		}
		if (errno == ECONNREFUSED) {
			return (gethtbyname_r(host, hostd, __UNCONST(name), buffer, buflen, result));
		} else {
			return (0);
		}
	}
	return (getanswer_r(host, hostd, statp, buf, n, 0, buffer, buflen, result));
}

static int
_hvs_gethostbyaddr(host, hostd, statp, buf, addr, len, type, buffer, buflen, result)
	struct hostent *host;
	struct hostent_data *hostd;
	res_state statp;
	querybuf *buf;
	const char *addr;
	int len, type;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	const u_char *uaddr;
	char qbuf[MAXDNAME + 1];
	int n, rval;

	uaddr = (const u_char *)addr;
	if (type == AF_INET6 && len == IN6ADDRSZ &&
		    (IN6_IS_ADDR_LINKLOCAL((const struct in6_addr *)addr) ||
		     IN6_IS_ADDR_SITELOCAL((const struct in6_addr *)addr))) {
		return (0);
	}
	if (type == AF_INET6 && len == IN6ADDRSZ
			&& (IN6_IS_ADDR_V4MAPPED((const struct in6_addr *)addr)
					|| IN6_IS_ADDR_V4COMPAT((const struct in6_addr *)addr))) {
		/* Unmap. */
		uaddr += IN6ADDRSZ - INADDRSZ;
		addr = (const char *)uaddr;
		type = AF_INET;
		len = INADDRSZ;
	}

	n = res_nquery(statp, qbuf, C_IN, T_PTR, (u_char *)buf, sizeof(buf));
	if (n < 0) {
		if (statp->options & RES_DEBUG) {
			printf("res_search failed\n");
		}
		if (errno == ECONNREFUSED) {
			return (gethtbyaddr_r(host, hostd, addr, len, type, buffer, buflen, result));
		} else {
			return (0);
		}
	}
	rval = getanswer_r(host, hostd, statp, buf, n, 1, buffer, buflen, result);
	if (rval == 0) {
		return (rval);
	}
	host->h_addrtype = type;
	host->h_length = len;
	switch (host->h_addrtype) {
	case AF_INET:
		hostaddr[0] = (char *)&host_addr;
		hostaddr[1] = (char *)NULL;
		host_addr = *(struct in_addr *)__UNCONST(addr);
		break;
	case AF_INET6:
		hostaddr[0] = (char *)&host6_addr;
		hostaddr[1] = (char *)NULL;
		host6_addr = *(struct in6_addr *)__UNCONST(addr);
		break;
	default:
		errno = EINVAL;
		return (0);
	}
	if (type == AF_INET && (statp->options & RES_USE_INET6)) {
		map_v4v6_address((char *)(void *)host_addrs, (char *)(void *)host_addrs);
		host->h_addrtype = AF_INET6;
		host->h_length = IN6ADDRSZ;
	}

	host->h_addr_list = hostaddr;
	host->h_addr = hostaddr[0];
	return (1);
}

static int
_hvs_parsehtent(hp, hd, buffer, buflen)
	struct hostent *hp;
	struct hostent_data *hd;
	char *buffer;
	size_t buflen;
{
	char *p;
	register char *cp, **q;
	int af, len;

	if (hd->hostf == NULL && (hd->hostf = fopen(HOSTDB, "r")) == NULL) {
		return (0);
	}

again:
	if ((p = fgets(buffer, buflen - 1, hd->hostf)) == NULL) {
		return (0);
	}
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

	if (inet_pton(AF_INET6, p, (char *)(void *)host_addrs) > 0) {
		af = AF_INET6;
		len = IN6ADDRSZ;
	} else if (inet_pton(AF_INET, p, (char *)(void *)host_addrs) > 0) {
		res_state res = __res_get_state();
		if (res == NULL) {
			return (0);
		}
		if (res->options & RES_USE_INET6) {
			map_v4v6_address((char *)(void *)host_addrs,
					(char *)(void *)host_addrs);
			af = AF_INET6;
			len = IN6ADDRSZ;
		} else {
			af = AF_INET;
			len = INADDRSZ;
		}
		__res_put_state(res);
	} else {
		goto again;
	}
	if (hp->h_addrtype != 0 && hp->h_addrtype != af) {
		goto again;
	}
	if (hp->h_length != 0 && hp->h_length != len) {
		goto again;
	}
	hostaddr[0] = (char *)(void *)host_addrs;
	hostaddr[1] = NULL;
	hp->h_addr_list = hostaddr;
	hp->h_length = len;
	hp->h_addrtype = af;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	hp->h_name = cp;
	if (hd->aliases == NULL) {
		hd->maxaliases = MAXALIASES;
		hd->aliases = calloc(hd->maxaliases, sizeof(*hd->aliases));
		if (hd->aliases == NULL) {
			endhtent_r(hd);
			return (0);
		}
	}
	q = hp->h_aliases = hd->aliases;
	cp = any(cp, " \t");
	if (cp != NULL)
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &hd->aliases[hd->maxaliases - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (1);
}

static int
_hvs_start(hd)
	struct hostent_data *hd;
{
	int rval;

	if (hd->hostf == NULL) {
        hd->filename = HOSTDB;
		hd->hostf = fopen(HOSTDB, "r" );
		rval = 0;
	} else {
		rewind(hd->hostf);
		rval = 1;
	}
	return (rval);
}

static int
_hvs_end(hd)
	struct hostent_data *hd;
{
	if (hd->hostf && !hd->stayopen) {
		fclose(hd->hostf);
		hd->hostf = NULL;
	}
	if (hd->aliases) {
		free(hd->aliases);
		hd->aliases = NULL;
		hd->maxaliases = 0;
	}
	return (1);
}

static int
_hvs_sethtent(hd, stayopen)
	struct hostent_data *hd;
	int stayopen;
{
	hd->stayopen |= stayopen;
	return (_hvs_start(hd));
}

static int
_hvs_endhtent(hd)
	struct hostent_data *hd;
{
	hd->stayopen = 0;
	return (_hvs_end(hd));
}

static void
_hvs_sethtfile(hd, file)
	struct hostent_data *hd;
	const char *file;
{
	hd->filename = file;
}

static int
_hvs_gethtent(hp, hd, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	int rval;

	rval = _hvs_start(hd);
	if (rval != 1) {
		return (rval);
	}

	rval = _hvs_parsehtent(hp, hd, buffer, buflen);
	if (rval == 1) {
		result = &hp;
	}
	return (rval);
}

static int
gethostbyname_internal(hp, hd, statp, buf, name, type, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	res_state statp;
	querybuf *buf;
	const char *name;
	int type;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	int rval;

	rval = _hvs_gethostbyname(hp, hd, statp, buf, name, type, buffer, buflen, result);
	if (rval == 1) {
		result = &hp;
	}
	return (rval);
}

static int
gethostbyaddr_internal(hp, hd, statp, buf, addr, len, type, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	res_state statp;
	querybuf *buf;
	const char *addr;
	int len, type;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	int rval;

	rval = _hvs_gethostbyaddr(hp, hd, statp, buf, addr, len, type, buffer, buflen, result);
	if (rval == 1) {
		result = &hp;
	}
	return (rval);
}

static int
getanswer_r(hp, hd, statp, answer, anslen, iquery, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	res_state statp;
	querybuf *answer;
	int anslen;
	int iquery;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	int rval;

	rval = _hvs_getanswer(hp, hd, statp, answer, anslen, iquery, buffer, buflen);
	if (rval == 1) {
		result = &hp;
	}
	return (rval);
}

static struct hostent *
getanswer(answer, anslen, iquery)
	querybuf *answer;
	int anslen;
	int iquery;
{
	struct hostent *result;
	res_state statp;
	int rval;

	statp = __res_get_state();
	if (statp == NULL) {
		return (0);
	}
	rval = getanswer_r(&_hvs_host, &_hvs_hostd, statp, answer, anslen, iquery, _hvs_hostbuf, sizeof(_hvs_hostbuf), &result);
	__res_put_state(statp);
	return ((rval == 1) ? result : NULL);
}

/*
 * Public Methods
 */
int
gethostbyname2_r(hp, hd, name, type, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	const char *name;
	int type;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	res_state statp;
	querybuf buf;
	int rval;

	statp = __res_get_state();
	if (statp == NULL) {
		h_errno = NETDB_INTERNAL;
		return (0);
	}

	rval = gethostbyname_internal(hp, hd, statp, &buf, name, type, buffer, buflen, result);
	__res_put_state(statp);
	return (rval);
}

struct hostent *
gethostbyname2(name, type)
	const char *name;
	int type;
{
	struct hostent *result;
	int rval;

	rval = gethostbyname2_r(&_hvs_host, &_hvs_hostd, name, type, _hvs_hostbuf, sizeof(_hvs_hostbuf), &result);
	return ((rval == 1) ? result : NULL);
}

int
gethostbyname_r(hp, hd, name, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	const char *name;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	res_state statp;
	int rval;

	statp = __res_get_state();
	if (statp == NULL) {
		h_errno = NETDB_INTERNAL;
		return (0);
	}

	if (statp->options & RES_USE_INET6) {
		rval = gethostbyname2_r(hp, hd, name, AF_INET6, buffer, buflen, result);
	} else {
		rval = gethostbyname2_r(hp, hd, name, AF_INET, buffer, buflen, result);
	}
	__res_put_state(statp);
	return (rval);
}

struct hostent *
gethostbyname(name)
	const char *name;
{
	struct hostent *result;
	int rval;

	rval = gethostbyname_r(&_hvs_host, &_hvs_hostd, name, _hvs_hostbuf, sizeof(_hvs_hostbuf), &result);
	return ((rval == 1) ? result : NULL);
}

int
gethostbyaddr_r(hp, hd, addr, len, type, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	const char *addr;
	int len, type;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	res_state statp;
	querybuf buf;
	int rval;

	statp = __res_get_state();
	if (statp == NULL) {
		h_errno = NETDB_INTERNAL;
		return (0);
	}

	rval = gethostbyaddr_internal(hp, hd, statp, &buf, addr, len, type, buffer, buflen, result);
	__res_put_state(statp);
	return (rval);
}

struct hostent *
gethostbyaddr(addr, len, type)
	const char *addr;
	int len, type;
{
	struct hostent *result;
	int rval;

	rval = gethostbyaddr_r(&_hvs_host, &_hvs_hostd, addr, len, type, _hvs_hostbuf, sizeof(_hvs_hostbuf), &result);
	return ((rval == 1) ? result : NULL);
}

int
gethtent_r(host, hostd, buffer, buflen, result)
	struct hostent *host;
	struct hostent_data *hostd;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	int rval;

	mutex_lock(&_hvsmutex);
	rval = _hvs_gethtent(host, hostd, buffer, buflen, result);
	mutex_unlock(&_hvsmutex);
	return (rval);
}

struct hostent *
gethtent(void)
{
	struct hostent *result;
	int rval;

	rval = gethtent_r(&_hvs_host, &_hvs_hostd, _hvs_hostbuf, sizeof(_hvs_hostbuf), &result);
	return ((rval == 1) ? result : NULL);
}

void
sethtent_r(f, hostd)
	int f;
	struct hostent_data *hostd;
{
	mutex_lock(&_hvsmutex);
	(void)_hvs_sethtent(hostd, f);
	mutex_unlock(&_hvsmutex);
}

void
endhtent_r(hostd)
	struct hostent_data *hostd;
{
	mutex_lock(&_hvsmutex);
	(void)_hvs_endhtent(hostd);
	mutex_unlock(&_hvsmutex);
}

void
sethtfile_r(name, hostd)
	const char *name;
	struct hostent_data *hostd;
{
	_hvs_sethtfile(hostd, name);
}

void
sethtent(f)
	int f;
{
	sethtent_r(f, &_hvs_hostd);
}

void
endhtent(void)
{
	endhtent_r(&_hvs_hostd);
}

void
sethtfile(name)
	const char *name;
{
	sethtfile_r(name, &_hvs_hostd);
}

int
gethtbyname_r(hp, hd, name, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	char *name;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	register char **cp;
	int rval;

	sethtent_r(hd->stayopen, hd);
	while ((rval = gethtent_r(hp, hd, buffer, buflen, result))) {
		if (strcasecmp(hp->h_name, name) == 0) {
			break;
		}
		for (cp = hp->h_aliases; *cp != 0; cp++) {
			if (strcasecmp(*cp, name) == 0) {
				goto found;
			}
		}
	}
found:
	endhtent_r(hd);
	return (rval);
}

int
gethtbyaddr_r(hp, hd, addr, len, type, buffer, buflen, result)
	struct hostent *hp;
	struct hostent_data *hd;
	const char *addr;
	int len, type;
	char *buffer;
	size_t buflen;
	struct hostent **result;
{
	int rval;

	sethtent_r(hd->stayopen, hd);
	while ((rval = gethtent_r(hp, hd, buffer, buflen, result))) {
		if (hp->h_addrtype == type && !bcmp(hp->h_addr, addr, len)) {
			break;
		}
	}
	endhtent_r(hd);
	return (rval);
}

struct hostent *
gethtbyname(name)
	char *name;
{
	struct hostent *p;
	int rval;

	rval = gethtbyname_r(&_hvs_host, &_hvs_hostd, name, _hvs_hostbuf, sizeof(_hvs_hostbuf), &p);
	return ((rval == 1) ? p : NULL);
}

struct hostent *
gethtbyaddr(addr, len, type)
	const char *addr;
	int len, type;
{
	struct hostent *p;
	int rval;

	rval = gethtbyaddr_r(&_hvs_host, &_hvs_hostd, addr, len, type, _hvs_hostbuf, sizeof(_hvs_hostbuf), &p);
	return ((rval == 1) ? p : NULL);
}

static void
map_v4v6_address(src, dst)
	const char *src;
	char *dst;
{
	u_char *p = (u_char*) dst;
	char tmp[INADDRSZ];
	int i;

	/* Stash a temporary copy so our caller can update in place. */
	(void) memcpy(tmp, src, INADDRSZ);
	/* Mark this ipv6 addr as a mapped ipv4. */
	for (i = 0; i < 10; i++)
		*p++ = 0x00;
	*p++ = 0xff;
	*p++ = 0xff;
	/* Retrieve the saved copy and we're done. */
	(void) memcpy((void*) p, tmp, INADDRSZ);
}

static void
map_v4v6_hostent(hp, bpp, ep)
	struct hostent *hp;
	char **bpp;
	char *ep;
{
	char **ap;

	if (hp->h_addrtype != AF_INET || hp->h_length != INADDRSZ)
		return;
	hp->h_addrtype = AF_INET6;
	hp->h_length = IN6ADDRSZ;
	for (ap = hp->h_addr_list; *ap; ap++) {
		int i = sizeof(align) - (size_t)((u_long)*bpp % sizeof(align));

		if (ep - *bpp < (i + IN6ADDRSZ)) {
			/* Out of memory.  Truncate address list here.  XXX */
			*ap = NULL;
			return;
		}
		*bpp += i;
		map_v4v6_address(*ap, *bpp);
		*ap = *bpp;
		*bpp += IN6ADDRSZ;
	}
}

static void
addrsort(ap, num, statp)
	char **ap;
	int num;
	res_state statp;
{
	int i, j;
	char **p;
	short aval[MAXADDRS];
	int needsort = 0;

	p = ap;
	for (i = 0; i < num; i++, p++) {
		for (j = 0; (unsigned) j < statp->nsort; j++) {
			if (statp->sort_list[j].addr.s_addr
					== (((struct in_addr*) (void*) (*p))->s_addr
							& statp->sort_list[j].mask)) {
				break;
			}
		}
		aval[i] = j;
		if (needsort == 0 && i > 0 && j < aval[i - 1]) {
			needsort = i;
		}
	}

	if (!needsort) {
		return;
	}

	while (needsort < num) {
		for (j = needsort - 1; j >= 0; j--) {
			if (aval[j] > aval[j + 1]) {
				char *hp;

				i = aval[j];
				aval[j] = aval[j + 1];
				aval[j + 1] = i;

				hp = ap[j];
				ap[j] = ap[j + 1];
				ap[j + 1] = hp;
			} else {
				break;
			}
		}
		needsort++;
	}
}

static char *
any(cp, match)
	register char *cp;
	const char *match;
{
	return (strpbrk(cp, match));
}
