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
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <util.h>

#include "ifconfig.h"

struct in_aliasreq in_addreq;

void	in_alias(struct ifreq *);
void	in_status(int);
void 	in_getaddr(const char *, int);
void 	in_getprefix(const char *, int);

struct afswtch af_inet = {
		.af_name = "inet",
		.af_af = AF_INET,
		.af_status  = in_status,
		.af_getaddr = in_getaddr,
		.af_getprefix = in_getprefix,
		.af_difaddr = SIOCDIFADDR,
		.af_aifaddr = SIOCAIFADDR,
		.af_gifaddr = SIOCGIFADDR,
		.af_ridreq = &ridreq,
		.af_addreq = &in_addreq,
};

void
inet_init(void)
{
	af_register(&af_inet);
}

void
in_alias(struct ifreq *creq)
{
	struct sockaddr_in *iasin;
	int alias;

	if (lflag)
		return;

	alias = 1;

	/* Get the non-alias address for this interface. */
	getsock(AF_INET);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(EXIT_FAILURE, "socket");
	}
	(void)memset(&ifr, 0, sizeof(ifr));
	(void)strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, &ifr) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			return;
		} else
			warn("SIOCGIFADDR");
	}
	/* If creq and ifr are the same address, this is not an alias. */
	if (memcmp(&ifr.ifr_addr, &creq->ifr_addr,
		   sizeof(creq->ifr_addr)) == 0)
		alias = 0;
	(void)memset(&in_addreq, 0, sizeof(in_addreq));
	(void)strncpy(in_addreq.ifra_name, name, sizeof(in_addreq.ifra_name));
	memcpy(&in_addreq.ifra_addr, &creq->ifr_addr,
	    sizeof(in_addreq.ifra_addr));
	if (ioctl(s, SIOCGIFALIAS, &in_addreq) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			return;
		} else
			warn("SIOCGIFALIAS");
	}

	iasin = &in_addreq.ifra_addr;
	printf("\tinet %s%s", alias ? "alias " : "", inet_ntoa(iasin->sin_addr));

	if (flags & IFF_POINTOPOINT) {
		iasin = &in_addreq.ifra_dstaddr;
		printf(" -> %s", inet_ntoa(iasin->sin_addr));
	}

	iasin = &in_addreq.ifra_mask;
	printf(" netmask 0x%x", ntohl(iasin->sin_addr.s_addr));

	if (flags & IFF_BROADCAST) {
		iasin = &in_addreq.ifra_broadaddr;
		printf(" broadcast %s", inet_ntoa(iasin->sin_addr));
	}
	printf("\n");
}

void
in_status(int force)
{
	struct ifaddrs *ifap, *ifa;
	struct ifreq isifr;

	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "getifaddrs");
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (strcmp(name, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		if (sizeof(isifr.ifr_addr) < ifa->ifa_addr->sa_len)
			continue;

		memset(&isifr, 0, sizeof(isifr));
		strncpy(isifr.ifr_name, ifa->ifa_name, sizeof(isifr.ifr_name));
		memcpy(&isifr.ifr_addr, ifa->ifa_addr, ifa->ifa_addr->sa_len);
		in_alias(&isifr);
	}
	freeifaddrs(ifap);
}


#define SIN(x) ((struct sockaddr_in *) &(x))
struct sockaddr_in *sintab[] = {
    SIN(ridreq.ifr_addr), SIN(in_addreq.ifra_addr),
    SIN(in_addreq.ifra_mask), SIN(in_addreq.ifra_broadaddr)};

void
in_getaddr(const char *str, int which)
{
	struct sockaddr_in *gasin = sintab[which];
	struct hostent *hp;
	struct netent *np;

	gasin->sin_len = sizeof(*gasin);
	if (which != MASK)
		gasin->sin_family = AF_INET;

	if (which == ADDR) {
		char *p = NULL;
		if ((p = strrchr(str, '/')) != NULL) {
			*p = '\0';
			in_getprefix(p + 1, MASK);
		}
	}

	if (inet_aton(str, &gasin->sin_addr) == 0) {
		if ((hp = gethostbyname(str)) != NULL)
			(void) memcpy(&gasin->sin_addr, hp->h_addr, hp->h_length);
		else if ((np = getnetbyname(str)) != NULL)
			gasin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
		else
			errx(EXIT_FAILURE, "%s: bad value", str);
	}
}

void
in_getprefix(const char *plen, int which)
{
	register struct sockaddr_in *igsin = sintab[which];
	register u_char *cp;
	int len = strtol(plen, (char **)NULL, 10);

	if ((len < 0) || (len > 32))
		errx(EXIT_FAILURE, "%s: bad value", plen);
	igsin->sin_len = sizeof(*igsin);
	if (which != MASK)
		igsin->sin_family = AF_INET;
	if ((len == 0) || (len == 32)) {
		memset(&igsin->sin_addr, 0xff, sizeof(struct in_addr));
		return;
	}
	memset((void *)&igsin->sin_addr, 0x00, sizeof(igsin->sin_addr));
	for (cp = (u_char *)&igsin->sin_addr; len > 7; len -= 8)
		*cp++ = 0xff;
	if (len)
		*cp = 0xff << (8 - len);
}
