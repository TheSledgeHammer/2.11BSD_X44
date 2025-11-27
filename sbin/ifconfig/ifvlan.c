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
#include <net/if_ether.h>

#include <net/if_vlanvar.h>

#include <ctype.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ifconfig.h"

u_int	vlan_tag = (u_int)-1;

void	setvlan(const char *, int);
void	setvlanif(const char *, int);
void	unsetvlanif(const char *, int);

const struct cmd vlan_cmds[] = {
		{ "vlan",	NEXTARG,	0,		setvlan } ,
		{ "vlanif",	NEXTARG,	0,		setvlanif } ,
		{ "-vlanif",	0,		0,		unsetvlanif } ,
};

void
vlan_init(void)
{
	size_t i;
	for (i = 0; i < nitems(vlan_cmds);  i++) {
		cmd_register(&vlan_cmds[i]);
	}
}

void
setvlan(const char *val, int d)
{
	struct vlanreq vlr;

	if (strncmp(ifr.ifr_name, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifr.ifr_name[4]))
		errx(EXIT_FAILURE,
		    "``vlan'' valid only with vlan(4) interfaces");

	vlan_tag = atoi(val);

	memset(&vlr, 0, sizeof(vlr));
	ifr.ifr_data = (void *)&vlr;

	if (ioctl(s, SIOCGETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGETVLAN");

	vlr.vlr_tag = vlan_tag;

	if (ioctl(s, SIOCSETVLAN, &ifr) == -1) {
		err(EXIT_FAILURE, "SIOCSETVLAN");
	}
}

void
setvlanif(const char *val, int d)
{
	struct vlanreq vlr;

	if (strncmp(ifr.ifr_name, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifr.ifr_name[4]))
		errx(EXIT_FAILURE,
		    "``vlanif'' valid only with vlan(4) interfaces");

	if (vlan_tag == (u_int)-1)
		errx(EXIT_FAILURE,
		    "must specify both ``vlan'' and ``vlanif''");

	memset(&vlr, 0, sizeof(vlr));
	ifr.ifr_data = (void *)&vlr;

	if (ioctl(s, SIOCGETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGETVLAN");

	strlcpy(vlr.vlr_parent, val, sizeof(vlr.vlr_parent));
	vlr.vlr_tag = vlan_tag;

	if (ioctl(s, SIOCSETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSETVLAN");
}

void
unsetvlanif(const char *val, int d)
{
	struct vlanreq vlr;

	if (strncmp(ifr.ifr_name, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifr.ifr_name[4]))
		errx(EXIT_FAILURE,
		    "``vlanif'' valid only with vlan(4) interfaces");

	memset(&vlr, 0, sizeof(vlr));
	ifr.ifr_data = (void *)&vlr;

	if (ioctl(s, SIOCGETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGETVLAN");

	vlr.vlr_parent[0] = '\0';
	vlr.vlr_tag = 0;

	if (ioctl(s, SIOCSETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSETVLAN");
}

void
vlan_status(void)
{
	struct vlanreq vlr;

	if (strncmp(ifr.ifr_name, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifr.ifr_name[4]))
		return;

	memset(&vlr, 0, sizeof(vlr));
	ifr.ifr_data = (void *)&vlr;

	if (ioctl(s, SIOCGETVLAN, &ifr) == -1) {
		return;
	}

	if (vlr.vlr_tag || vlr.vlr_parent[0] != '\0')
		printf("\tvlan: %d parent: %s\n",
		    vlr.vlr_tag, vlr.vlr_parent[0] == '\0' ?
		    "<none>" : vlr.vlr_parent);
}
