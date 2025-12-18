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

#define	NEXTARG			0xffffff
#define	NEXTARG2		0xfffffe

struct cmd {
	const char 	*c_name;
	int			c_parameter;	/* NEXTARG means next argv */
	int			c_action;		/* defered action */
	void		(*c_func)(const char *, int);
	void		(*c_func2)(const char *, const char *);
	struct cmd 	*c_next;
};

void cmd_register(struct cmd *p);
struct cmd *cmd_lookup(const char *name);

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
	struct afswtch 	*af_next;
};

void af_register(struct afswtch *);
struct afswtch *af_getbyname(const char *);
struct afswtch *af_getbyfamily(int);

#define RIDADDR 0
#define ADDR	1
#define MASK	2
#define DSTADDR	3

extern struct ifreq			ifr, ridreq;
extern struct ifaliasreq	addreq __attribute__((aligned(4)));
extern struct in_aliasreq	in_addreq;
extern struct afswtch		*afp;
#ifdef INET6
extern struct in6_ifreq		ifr6;
extern struct in6_ifreq		in6_ridreq;
extern struct in6_aliasreq	in6_addreq;
#endif

extern char 				name[30];
extern int 					s;
extern int 					explicit_prefix;
extern int					actions;			/* Actions performed */

extern u_short 				flags;
extern int 					lflag;
extern int 					zflag;
#ifdef INET6
extern int 					Lflag;
#endif /* INET6 */

void getsock(int);
int  getinfo(struct ifreq *);
const char *get_string(const char *, const char *, u_int8_t *, int *);
void print_string(const u_int8_t *, int);

/* af_inet */
void inet_init(void);
void in_status(int);

/* af_inet6 */
#ifdef INET6
struct sockaddr_in6;
void inet6_init(void);
void in6_init(void);
void in6_status(int);
void in6_fillscopeid(struct sockaddr_in6 *);
#endif

/* af_ns */
void xns_init(void);
void xns_status(int);
void xns_nsip(int, int);

/* ifcarp */
void carp_init(void);
void carp_status(void);

/* ifieee80211 */
void ieee80211_init(void);
void ieee80211_status(void);

/* ifmedia */
void media_init(void);
void process_media_commands(void);
void print_media_word(int, const char *);
int  carrier(void);

/* iftunnel */
void tunnel_init(void);
void tunnel_status(void);

/* ifvlan */
void vlan_init(void);
void vlan_status(void);
#endif /* IFCONFIG_IFCONFIG_H */
