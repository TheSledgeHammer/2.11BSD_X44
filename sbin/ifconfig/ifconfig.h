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

#ifndef IFCONFIG_IFCONFIG_H
#define IFCONFIG_IFCONFIG_H

#define	A_MEDIA			0x0001		/* media command */
#define	A_MEDIAOPTSET	0x0002		/* mediaopt command */
#define	A_MEDIAOPTCLR	0x0004		/* -mediaopt command */
#define	A_MEDIAOPT		(A_MEDIAOPTSET|A_MEDIAOPTCLR)
#define	A_MEDIAINST		0x0008		/* instance or inst command */
#define	A_MEDIAMODE		0x0010		/* mode command */

#define	NEXTARG			0xffffff
#define	NEXTARG2		0xfffffe

struct cmd_head;
LIST_HEAD(cmd_head, cmd);
const struct cmd {
	const char 	*c_name;
	int			c_parameter;	/* NEXTARG means next argv */
	int			c_action;		/* defered action */
	void		(*c_func)(const char *, int);
	void		(*c_func2)(const char *, const char *);

	//struct cmd 	*c_next;
	LIST_ENTRY(cmd) c_next;
};

void cmd_register(const struct cmd *p);
const struct cmd *cmd_lookup(const char *name);

struct afswtch_head;
LIST_HEAD(afswtch_head, afswtch);
/* Known address families */
struct afswtch {
	const char 		*af_name;
	short 			af_af;
	void 			(*af_status)(int);
	void 			(*af_getaddr)(const char *, int);
	void 			(*af_getprefix)(const char *, int);
	u_long 			af_difaddr;
	u_long 			af_aifaddr;
	u_long 			af_gifaddr;
	void 			*af_ridreq;
	void 			*af_addreq;
	//struct afswtch 	*af_next;
	LIST_ENTRY(afswtch) af_next;
};

void af_register(struct afswtch *p);
struct afswtch *af_getbyname(const char *name);
struct afswtch *af_getbyfamily(int af);

#define RIDADDR 0
#define ADDR	1
#define MASK	2
#define DSTADDR	3

extern struct ifreq			ifr, ridreq;
extern struct ifaliasreq	addreq __attribute__((aligned(4)));
extern struct in_aliasreq	in_addreq;
#ifdef INET6
extern struct in6_ifreq		ifr6;
extern struct in6_ifreq		in6_ridreq;
extern struct in6_aliasreq	in6_addreq;
#endif
extern struct sockaddr_in	netmask;
extern struct ifcapreq 		g_ifcr;

extern char 				name[30];
extern u_short				flags;
extern int 					s;
extern int					actions;			/* Actions performed */

void getsock(int naf);
int  getinfo(struct ifreq *giifr);

/* af_inet */
void inet_init(void);

/* af_inet6 */
void inet6_init(void);

/* ifieee80211 */
void ieee80211_init(void);

/* ifmedia */
void media_init(void);
void process_media_commands(void);

/* ifns */
void xns_init(void);

/* ifvlan */
void vlan_init(void);
#endif /* IFCONFIG_IFCONFIG_H */
