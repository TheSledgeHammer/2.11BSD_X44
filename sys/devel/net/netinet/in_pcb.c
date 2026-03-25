/*	$NetBSD: in_pcb.c,v 1.94 2004/03/02 02:26:28 thorpej Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Public Access Networks Corporation ("Panix").  It was developed under
 * contract to Panix by Eric Haszlakiewicz and Thor Lancelot Simon.
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
 * Copyright (c) 1982, 1986, 1991, 1993, 1995
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
 *
 *	@(#)in_pcb.c	8.4 (Berkeley) 5/24/95
 */

#include <sys/null.h>

#include <devel/net/netinet/in_pcb.h>

#define INPCBHASH(laddr, lport, faddr, fport)	\
	(ntohl((laddr).s_addr) + ntohs(lport) + ntohl((faddr).s_addr) + ntohs(fport))

#define	INPCBHASH_LOCAL(table, laddr, lport, faddr, fport)  \
	(&(table)->inpt_localhashtbl[INPCBHASH((laddr), (lport), (faddr), (fport)) & table->inpt_localhash])

#define	INPCBHASH_FOREIGN(table, laddr, lport, faddr, fport)  \
	(&(table)->inpt_foreignhashtbl[INPCBHASH((laddr), (lport), (faddr), (fport)) & table->inpt_foreignhash])

static struct inpcb *in_pcbhashlookup(struct inpcbtable *, struct in_addr, u_int, struct in_addr, u_int, int, int);
static void in_pcbrehash(struct inpcbhead *, struct inpcb *, int);
static void in_pcbinsert(struct inpcbtable *, struct inpcb *, int);
static void in_pcbremove(struct inpcb *, int);

void
in_pcbinit(struct inpcbtable *table, int bindhashsize, int connecthashsize)
{
	CIRCLEQ_INIT(&table->inpt_queue);
	table->inpt_foreignhashtbl = hashinit(bindhashsize, M_PCB, &table->inpt_foreignhash);
	table->inpt_localhashtbl = hashinit(connecthashsize, M_PCB, &table->inpt_localhash);
	table->inpt_lastlow = IPPORT_RESERVEDMAX;
	table->inpt_lastport = (u_int16_t)anonportmax;
}

int
in_pcballoc(struct socket *so, void *v)
{
	struct inpcbtable *table = v;
	struct inpcb *inp;
	int s;
#if defined(IPSEC) || defined(FAST_IPSEC)
	int error;
#endif

	MALLOC(inp, struct inpcb *, sizeof(*inp), M_PCB, M_WAITOK);
	if (inp == NULL)
		return (ENOBUFS);
	bzero((caddr_t)inp, sizeof(*inp));
	inp->inp_af = AF_INET;
	inp->inp_table = table;
	inp->inp_socket = so;
	inp->inp_errormtu = -1;
#if defined(IPSEC) || defined(FAST_IPSEC)
	error = ipsec_init_pcbpolicy(so, &inp->inp_sp);
	if (error != 0) {
		FREE(inp, M_PCB);
		return error;
	}
#endif
	so->so_pcb = inp;
	s = splnet();
	CIRCLEQ_INSERT_HEAD(&table->inpt_queue, &inp->inp_head, inph_queue);
	in_pcbinsert(table, inp, INPLOOKUP_FOREIGN);
	in_pcbinsert(table, inp, INPLOOKUP_LOCAL);
	splx(s);
	return (0);
}

int
in_pcbbind(void *v, struct mbuf *nam, struct proc *p)
{
	struct in_ifaddr *ia = NULL;
	struct inpcb *inp = v;
	struct socket *so = inp->inp_socket;
	struct inpcbtable *table = inp->inp_table;
	struct sockaddr_in *sin;
	u_int16_t lport = 0;
	int wild = 0, reuseport = (so->so_options & SO_REUSEPORT);

	if (inp->inp_af != AF_INET)
		return (EINVAL);

	if (TAILQ_FIRST(&in_ifaddrhead) == 0)
		return (EADDRNOTAVAIL);
	if (inp->inp_lport || !in_nullhost(inp->inp_laddr))
		return (EINVAL);
	if ((so->so_options & (SO_REUSEADDR | SO_REUSEPORT)) == 0)
		wild = 1;
	if (nam == 0)
		goto noname;
	sin = mtod(nam, struct sockaddr_in *);
	if (nam->m_len != sizeof(*sin))
		return (EINVAL);
	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	lport = sin->sin_port;
	if (IN_MULTICAST(sin->sin_addr.s_addr)) {
		/*
		 * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
		 * allow complete duplication of binding if
		 * SO_REUSEPORT is set, or if SO_REUSEADDR is set
		 * and a multicast address is bound on both
		 * new and duplicated sockets.
		 */
		if (so->so_options & SO_REUSEADDR)
			reuseport = SO_REUSEADDR | SO_REUSEPORT;
	} else if (!in_nullhost(sin->sin_addr)) {
		sin->sin_port = 0; /* yech... */
		INADDR_TO_IA(sin->sin_addr, ia);
		/* check for broadcast addresses */
		if (ia == NULL)
			ia = ifatoia(ifa_ifwithaddr(sintosa(sin)));
		if (ia == NULL)
			return (EADDRNOTAVAIL);
	}
	if (lport) {
		struct inpcb *t;
#ifdef INET6
		struct in6pcb *t6;
		struct in6_addr mapped;
#endif
#ifndef IPNOPRIVPORTS
		/* GROSS */
		if (ntohs(lport) < IPPORT_RESERVED
				&& (p == 0 || suser1(p->p_ucred, &p->p_acflag)))
			return (EACCES);
#endif
#ifdef INET6
		memset(&mapped, 0, sizeof(mapped));
		mapped.s6_addr16[5] = 0xffff;
		memcpy(&mapped.s6_addr32[3], &sin->sin_addr,
				sizeof(mapped.s6_addr32[3]));
		t6 = in6_pcblookup_port(table, &zeroin6_addr, 0, &mapped, lport, wild);
		if (t6 && (reuseport & t6->in6p_socket->so_options) == 0)
			return (EADDRINUSE);
#endif
		if (so->so_uid && !IN_MULTICAST(sin->sin_addr.s_addr)) {
			t = in_pcblookup_port(table, sin->sin_addr, lport, 1);
			/*
			 * XXX:	investigate ramifications of loosening this
			 *	restriction so that as long as both ports have
			 *	SO_REUSEPORT allow the bind
			 */
			if (t
					&& (!in_nullhost(sin->sin_addr)
							|| !in_nullhost(t->inp_laddr)
							|| (t->inp_socket->so_options & SO_REUSEPORT) == 0)
					&& (so->so_uid != t->inp_socket->so_uid)) {
				return (EADDRINUSE);
			}
		}
		t = in_pcblookup_port(table, zeroin_addr, 0, sin->sin_addr, lport, wild);
		if (t && (reuseport & t->inp_socket->so_options) == 0)
			return (EADDRINUSE);
	}
	inp->inp_laddr = sin->sin_addr;

noname:
	if (lport == 0) {
		int cnt;
		u_int16_t min, max;
		u_int16_t *lastport;

		if (inp->inp_flags & INP_LOWPORT) {
#ifndef IPNOPRIVPORTS
			if (p == 0 || suser1(p->p_ucred, &p->p_acflag))
				return (EACCES);
#endif
			min = lowportmin;
			max = lowportmax;
			lastport = &table->inpt_lastlow;
		} else {
			min = anonportmin;
			max = anonportmax;
			lastport = &table->inpt_lastport;
		}
		if (min > max) { /* sanity check */
			u_int16_t swp;

			swp = min;
			min = max;
			max = swp;
		}

		lport = *lastport - 1;
		for (cnt = max - min + 1; cnt; cnt--, lport--) {
			if (lport < min || lport > max)
				lport = max;
			if (!in_pcblookup_port(table, zeroin_addr, 0, inp->inp_laddr, htons(lport), 1))
				goto found;
		}
		if (!in_nullhost(inp->inp_laddr))
			inp->inp_laddr.s_addr = INADDR_ANY;
		return (EAGAIN);
found:
		inp->inp_flags |= INP_ANONPORT;
		*lastport = lport;
		lport = htons(lport);
	}
	inp->inp_lport = lport;
	in_pcbremove(inp, INPLOOKUP_FOREIGN);
	in_pcbremove(inp, INPLOOKUP_LOCAL);
	in_pcbinsert(table, inp, INPLOOKUP_FOREIGN);
	in_pcbinsert(table, inp, INPLOOKUP_LOCAL);
	return (0);
}

struct inpcb *
in_pcblookup_port(struct inpcbtable *table, struct in_addr faddr, u_int fport_arg, struct in_addr laddr, u_int lport_arg, int lookup_wildcard)
{
	u_int16_t fport = fport_arg, lport = lport_arg;

	return (in_pcblookup_local(table, faddr, fport, laddr, lport, lookup_wildcard));
}

struct inpcb *
in_pcblookup_connect(struct inpcbtable *table, struct in_addr faddr, u_int fport_arg, struct in_addr laddr, u_int lport_arg)
{
	u_int16_t fport = fport_arg, lport = lport_arg;

	return (in_pcblookup_foreign(table, faddr, fport, laddr, lport));
}

struct inpcb *
in_pcblookup_bind(struct inpcbtable *table, struct in_addr faddr, u_int fport_arg, struct in_addr laddr, u_int lport_arg)
{
	struct inpcb *inp;
	u_int16_t fport = fport_arg, lport = lport_arg;

	inp = in_pcblookup_foreign(table, faddr, fport, laddr, lport);
	if (inp != NULL) {
		return (inp);
	}
	inp = in_pcblookup_foreign(table, faddr, fport, zeroin_addr, lport);
	if (inp != NULL) {
		return (inp);
	}
	return (NULL);
}

void
in_pcbstate(struct inpcb *inp, int state, int which)
{
	if (inp->inp_af != AF_INET) {
		return;
	}
	if (inp->inp_state > INP_ATTACHED) {
		in_pcbremove(inp, INPLOOKUP_FOREIGN);
		in_pcbremove(inp, INPLOOKUP_LOCAL);
	}

	switch (state) {
	case INP_BOUND:
		in_pcbinsert(inp->inp_table, inp, INPLOOKUP_FOREIGN);
		in_pcbinsert(inp->inp_table, inp, INPLOOKUP_LOCAL);
		break;
	case INP_CONNECTED:
		in_pcbinsert(inp->inp_table, inp, INPLOOKUP_FOREIGN);
		in_pcbinsert(inp->inp_table, inp, INPLOOKUP_LOCAL);
		break;
	}
	inp->inp_state = state;
}

struct inpcb *
in_pcblookup_local(struct inpcbtable *table, struct in_addr faddr, u_int fport_arg, struct in_addr laddr, u_int lport_arg, int lookup_wildcard)
{
	u_int16_t fport = fport_arg, lport = lport_arg;

	return (in_pcbhashlookup(table, faddr, fport, laddr, lport, lookup_wildcard, INPLOOKUP_LOCAL));
}

struct inpcb *
in_pcblookup_foreign(struct inpcbtable *table, struct in_addr faddr, u_int fport_arg, struct in_addr laddr, u_int lport_arg)
{
	u_int16_t fport = fport_arg, lport = lport_arg;

	return (in_pcbhashlookup(table, faddr, fport, laddr, lport, 1, INPLOOKUP_FOREIGN));
}

/*
 * in_pcbhashlookup:
 * - combines elements from 4.4BSD-lite2 and NetBSD.
 * - changes: (from  4.4BSD-lite2 and/or NetBSD)
 * 	 - 2 hashtables: foreign and local.
 * 	 - unified lookup:
 * 	 	- which: determines the hashtable to traverse
 * 	 - simplifies lookup_bind, lookup_connect and lookup_port functions
 */
static struct inpcb *
in_pcbhashlookup(struct inpcbtable *table, struct in_addr faddr, u_int fport_arg, struct in_addr laddr, u_int lport_arg, int lookup_wildcard, int which)
{
	struct inpcbhead *head;
	struct inpcb_hdr *inph;
	struct inpcb *inp, *match;
	int matchwild = 3, wildcard;
	u_int16_t fport = fport_arg, lport = lport_arg;

	switch (which) {
	case INPLOOKUP_FOREIGN:
		head = INPCBHASH_FOREIGN(table, laddr, lport, faddr, fport);
		LIST_FOREACH(inph, head, inph_fhash) {
			inp = (struct inpcb *)inph;
			if (inp->inp_af != AF_INET) {
				continue;
			}

			if (in_hosteq(inp->inp_faddr, faddr) &&
			    inp->inp_fport == fport &&
			    inp->inp_lport == lport &&
			    in_hosteq(inp->inp_laddr, laddr)) {
				goto out;
			}
		}
		break;

	case INPLOOKUP_LOCAL:
		head = INPCBHASH_LOCAL(table, laddr, lport, faddr, fport);
		LIST_FOREACH(inph, head, inph_lhash) {
			inp = (struct inpcb *)inph;
			if (inp->inp_af != AF_INET) {
				continue;
			}
			if (inp->inp_lport != lport) {
				continue;
			}
			wildcard = 0;
			if (inp->inp_faddr.s_addr != INADDR_ANY) {
				if (!in_nullhost(inp->inp_faddr)) {
					wildcard++;
				} else if (inp->inp_faddr.s_addr != faddr.s_addr
						|| inp->inp_fport != fport) {
					continue;
				}
			} else {
				if (faddr.s_addr != INADDR_ANY) {
					wildcard++;
				}
			}
			if (inp->inp_laddr.s_addr != INADDR_ANY) {
				if (in_hosteq(inp->inp_laddr, laddr)) {
					wildcard++;
				} else if (inp->inp_laddr.s_addr != laddr.s_addr) {
					continue;
				}
			} else {
				if (laddr.s_addr != INADDR_ANY) {
					wildcard++;
				}
			}
			if (wildcard && !lookup_wildcard) {
				continue;
			}
			if (wildcard < matchwild) {
				match = inp;
				matchwild = wildcard;
				if (matchwild == 0) {
					goto found;
					//break;
				}
			}
		}
		goto found;

	default:
		break;
	}
#ifdef DIAGNOSTIC
	if (in_pcbnotifymiss) {
		printf("in_pcbhashlookup: faddr=%08x fport=%d laddr=%08x lport=%d\n",
		    ntohl(faddr.s_addr), ntohs(fport),
		    ntohl(laddr.s_addr), ntohs(lport));
	}
#endif
	return (NULL);

out:
	in_pcbrehash(head, inp, which);
	return (inp);

found:
	return (match);
}

static void
in_pcbrehash(struct inpcbhead *head, struct inpcb *inp, int which)
{
	struct inpcb_hdr *inph;

	/* Move this PCB to the head of hash chain. */
	inph = &inp->inp_head;
	if (inph != LIST_FIRST(head)) {
		switch (which) {
		case INPLOOKUP_FOREIGN:
			LIST_REMOVE(inph, inph_hash);
			LIST_INSERT_HEAD(head, inph, inph_fhash);
			break;
		case INPLOOKUP_LOCAL:
			LIST_REMOVE(inph, inph_lhash);
			LIST_INSERT_HEAD(head, inph, inph_lhash);
			break;
		}
	}
}

static void
in_pcbinsert(struct inpcbtable *table, struct inpcb *inp, int which)
{
	switch (which) {
	case INPLOOKUP_FOREIGN:
		LIST_INSERT_HEAD(
				INPCBHASH_FOREIGN(table, inp->inp_faddr, inp->inp_fport, inp->inp_laddr, inp->inp_lport),
				&inp->inp_head, inph_fhash);
		break;
	case INPLOOKUP_LOCAL:
		LIST_INSERT_HEAD(
				INPCBHASH_LOCAL(table, inp->inp_faddr, inp->inp_fport, inp->inp_laddr, inp->inp_lport),
				&inp->inp_head, inph_lhash);
		break;
	}
}

static void
in_pcbremove(struct inpcb *inp, int which)
{
	switch (which) {
	case INPLOOKUP_FOREIGN:
		LIST_REMOVE(&inp->inp_head, inph_fhash);
		break;
	case INPLOOKUP_LOCAL:
		LIST_REMOVE(&inp->inp_head, inph_lhash);
		break;
	}
}
