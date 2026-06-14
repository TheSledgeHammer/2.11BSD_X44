/*	$NetBSD: af_iso.c,v 1.14 2010/12/13 17:35:08 pooka Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
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
#ifndef lint
__RCSID("$NetBSD: af_iso.c,v 1.14 2010/12/13 17:35:08 pooka Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>

#define EON
#include <netiso/iso.h>
#include <netiso/iso_var.h>

#include <err.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include "ifconfig.h"

struct iso_ifreq iso_ridreq;
struct iso_aliasreq iso_addreq;
int	nsellength = 1;

void setsnpaoffset(const char *, int);
void setnsellength(const char *, int);

struct cmd iso_cmds[] = {
		{ "snpaoffset",	NEXTARG, 0, setsnpaoffset },
		{ "nsellength",	NEXTARG, 0,	setnsellength },
};

void iso_getaddr(const char *, int);
void fixnsel(struct sockaddr_iso *);

struct afswtch af_iso = {
		.af_name	= "iso",
		.af_af		= AF_ISO,
		.af_status  = iso_status,
		.af_getaddr = iso_getaddr,
		.af_getprefix = NULL,
		.af_difaddr = SIOCDIFADDR_ISO,
		.af_aifaddr = SIOCAIFADDR_ISO,
		.af_gifaddr = SIOCGIFADDR_ISO,
		.af_ridreq = &iso_ridreq,
		.af_addreq = &iso_addreq,
};

#define SISO(x) ((struct sockaddr_iso *) &(x))
struct sockaddr_iso *sisotab[] = {
		SISO(iso_ridreq.ifr_Addr), SISO(iso_addreq.ifra_addr),
		SISO(iso_addreq.ifra_mask), SISO(iso_addreq.ifra_dstaddr)
};

void
iso_init(void)
{
	size_t i;

	for (i = 0; i < nitems(iso_cmds);  i++) {
		cmd_register(&iso_cmds[i]);
	}
	af_register(&af_iso);
}

void
setsnpaoffset(const char *val, int d)
{
	iso_addreq.ifra_snpaoffset = atoi(val);
}

void
setnsellength(const char *val, int d)
{
	nsellength = atoi(val);
	if (nsellength < 0)
		errx(1, "Negative NSEL length is absurd");
	if (afp == 0 || afp->af_af != AF_ISO)
		errx(1, "Setting NSEL length valid only for iso");
}

void
fixnsel(struct sockaddr_iso *siso)
{
	if (siso->siso_family == 0) {
		return;
	}
	siso->siso_tlen = nsellength;
}

void
adjust_nsellength(int af)
{
	if (af == AF_ISO) {
		fixnsel(sisotab[RIDADDR]);
		fixnsel(sisotab[ADDR]);
		fixnsel(sisotab[DSTADDR]);
	}
}

void
iso_getaddr(const char *addr, int which)
{
	register struct sockaddr_iso *siso = sisotab[which];

	siso->siso_addr = *iso_addr(addr);
	if (which == MASK) {
		siso->siso_len = TSEL(siso) - (caddr_t)(siso);
		siso->siso_nlen = 0;
	} else {
		siso->siso_len = sizeof(*siso);
		siso->siso_family = AF_ISO;
	}
}

void
iso_status(int force)
{
	struct sockaddr_iso *siso;
	struct iso_ifreq isoifr;

	getsock(AF_ISO);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(EXIT_FAILURE, "socket");
	}
	(void)memset(&isoifr, 0, sizeof(isoifr));
	(void)strncpy(isoifr.ifr_name, name, sizeof(isoifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR_ISO, &isoifr) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			(void)memset(&isoifr.ifr_Addr, 0,
			    sizeof(isoifr.ifr_Addr));
		} else
			warn("SIOCGIFADDR_ISO");
	}
	(void)strncpy(isoifr.ifr_name, name, sizeof isoifr.ifr_name);
	siso = &isoifr.ifr_Addr;
	printf("\tiso %s ", iso_ntoa(&siso->siso_addr));
	if (ioctl(s, SIOCGIFNETMASK_ISO, &isoifr) == -1) {
		if (errno == EADDRNOTAVAIL)
			memset(&isoifr.ifr_Addr, 0, sizeof(isoifr.ifr_Addr));
		else
			warn("SIOCGIFNETMASK_ISO");
	} else {
		if (siso->siso_len > offsetof(struct sockaddr_iso, siso_addr))
			siso->siso_addr.isoa_len = siso->siso_len
			    - offsetof(struct sockaddr_iso, siso_addr);
		printf("\n\t\tnetmask %s ", iso_ntoa(&siso->siso_addr));
	}
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR_ISO, &isoifr) == -1) {
			if (errno == EADDRNOTAVAIL)
			    memset(&isoifr.ifr_Addr, 0, sizeof(isoifr.ifr_Addr));
			else
			    warn("SIOCGIFDSTADDR_ISO");
		}
		(void)strncpy(isoifr.ifr_name, name, sizeof(isoifr.ifr_name));
		siso = &isoifr.ifr_Addr;
		printf("--> %s ", iso_ntoa(&siso->siso_addr));
	}
	printf("\n");
}
