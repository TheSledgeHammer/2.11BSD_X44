/*	$NetBSD: ifconfig.c,v 1.141.4.2 2005/07/24 01:58:38 snj Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>

#ifdef INET6
#include <netinet/in.h>
#endif

#include <ctype.h>
#include <err.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include "ifconfig.h"

void settunnel(const char *, const char *);
void deletetunnel(const char *, int);

const struct cmd tunnel_cmds[] = {
		{ "tunnel",	NEXTARG2,	0,		NULL, settunnel } ,
		{ "deletetunnel", 0,		0,		deletetunnel },
};

void
tunnel_init(void)
{
	size_t i;

	for (i = 0; i < nitems(tunnel_cmds);  i++) {
		cmd_register(&tunnel_cmds[i]);
	}
}

void
settunnel(const char *src, const char *dst)
{
	struct addrinfo hints, *srcres, *dstres;
	int ecode;
	struct if_laddrreq req;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = afp->af_af;
	hints.ai_socktype = SOCK_DGRAM;	/*dummy*/

	if ((ecode = getaddrinfo(src, NULL, &hints, &srcres)) != 0)
		errx(EXIT_FAILURE, "error in parsing address string: %s",
		    gai_strerror(ecode));

	if ((ecode = getaddrinfo(dst, NULL, &hints, &dstres)) != 0)
		errx(EXIT_FAILURE, "error in parsing address string: %s",
		    gai_strerror(ecode));

	if (srcres->ai_addr->sa_family != dstres->ai_addr->sa_family)
		errx(EXIT_FAILURE,
		    "source and destination address families do not match");

	if (srcres->ai_addrlen > sizeof(req.addr) ||
	    dstres->ai_addrlen > sizeof(req.dstaddr))
		errx(EXIT_FAILURE, "invalid sockaddr");

	memset(&req, 0, sizeof(req));
	strncpy(req.iflr_name, name, sizeof(req.iflr_name));
	memcpy(&req.addr, srcres->ai_addr, srcres->ai_addrlen);
	memcpy(&req.dstaddr, dstres->ai_addr, dstres->ai_addrlen);

#ifdef INET6
	if (req.addr.ss_family == AF_INET6) {
		struct sockaddr_in6 *s6, *d;

		s6 = (struct sockaddr_in6 *)&req.addr;
		d = (struct sockaddr_in6 *)&req.dstaddr;
		if (s6->sin6_scope_id != d->sin6_scope_id) {
			errx(EXIT_FAILURE, "scope mismatch");
			/* NOTREACHED */
		}
#ifdef __KAME__
		/* embed scopeid */
		if (s6->sin6_scope_id &&
		    (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&s6->sin6_addr))) {
			*(u_int16_t *)&s6->sin6_addr.s6_addr[2] =
			    htons(s6->sin6_scope_id);
		}
		if (d->sin6_scope_id &&
		    (IN6_IS_ADDR_LINKLOCAL(&d->sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&d->sin6_addr))) {
			*(u_int16_t *)&d->sin6_addr.s6_addr[2] =
			    htons(d->sin6_scope_id);
		}
#endif
	}
#endif

	if (ioctl(s, SIOCSLIFPHYADDR, &req) == -1)
		warn("SIOCSLIFPHYADDR");

	freeaddrinfo(srcres);
	freeaddrinfo(dstres);
}

/* ARGSUSED */
void
deletetunnel(const char *vname, int param)
{

	if (ioctl(s, SIOCDIFPHYADDR, &ifr) == -1) {
		err(EXIT_FAILURE, "SIOCDIFPHYADDR");
	}
}

void
tunnel_status(void)
{
	char psrcaddr[NI_MAXHOST];
	char pdstaddr[NI_MAXHOST];
	const char *ver = "";
#ifdef NI_WITHSCOPEID
	const int niflag = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflag = NI_NUMERICHOST;
#endif
	struct if_laddrreq req;

	psrcaddr[0] = pdstaddr[0] = '\0';

	memset(&req, 0, sizeof(req));
	strncpy(req.iflr_name, name, IFNAMSIZ);
	if (ioctl(s, SIOCGLIFPHYADDR, &req) == -1) {
		return;
	}
#ifdef INET6
	if (req.addr.ss_family == AF_INET6)
		in6_fillscopeid((struct sockaddr_in6 *)&req.addr);
#endif
	getnameinfo((struct sockaddr *)&req.addr, req.addr.ss_len,
	    psrcaddr, sizeof(psrcaddr), 0, 0, niflag);
#ifdef INET6
	if (req.addr.ss_family == AF_INET6)
		ver = "6";
#endif

#ifdef INET6
	if (req.dstaddr.ss_family == AF_INET6)
		in6_fillscopeid((struct sockaddr_in6 *)&req.dstaddr);
#endif
	getnameinfo((struct sockaddr *)&req.dstaddr, req.dstaddr.ss_len,
	    pdstaddr, sizeof(pdstaddr), 0, 0, niflag);

	printf("\ttunnel inet%s %s --> %s\n", ver, psrcaddr, pdstaddr);
}
