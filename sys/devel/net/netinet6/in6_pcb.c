/*	$NetBSD: in6_pcb.c,v 1.61.2.1 2004/04/28 05:56:07 jmc Exp $	*/
/*	$KAME: in6_pcb.c,v 1.84 2001/02/08 18:02:08 itojun Exp $	*/

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

/*
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)in_pcb.c	8.2 (Berkeley) 1/4/94
 */

#include <sys/null.h>

#include <devel/net/netinet6/in6_pcb.h>

#define IN6PCBHASH(laddr, lport, faddr, fport)	\
	((((faddr)->s6_addr32[0] ^ (faddr)->s6_addr32[1] ^ \
	  (faddr)->s6_addr32[2] ^ (faddr)->s6_addr32[3]) + ntohs(fport)) + \
	(((laddr)->s6_addr32[0] ^ (laddr)->s6_addr32[1] ^ \
	  (laddr)->s6_addr32[2] ^ (laddr)->s6_addr32[3]) + ntohs(lport)))

#define	IN6PCBHASH_LOCAL(table, laddr, lport, faddr, fport)  \
	(&(table)->inpt_localhashtbl[IN6PCBHASH((laddr), (lport), (faddr), (fport)) & table->inpt_localhash])

#define	IN6PCBHASH_FOREIGN(table, laddr, lport, faddr, fport)  \
	(&(table)->inpt_foreignhashtbl[IN6PCBHASH((laddr), (lport), (faddr), (fport)) & table->inpt_foreignhash])

static struct in6pcb *in6_pcbhashlookup(struct inpcbtable *, struct in6_addr *, u_int, struct in6_addr *, u_int, int, int, int);
static void in6_pcbrehash(struct inpcbhead *, struct in6pcb *, int);
static void in6_pcbinsert(struct inpcbtable *, struct in6pcb *, int);
static void in6_pcbremove(struct in6pcb *, int);

void
in6_pcbinit(struct inpcbtable *table, int bindhashsize, int connecthashsize)
{
	in_pcbinit(table, bindhashsize, connecthashsize);
	table->inpt_lastport = (u_int16_t)ip6_anonportmax;
}

int
in6_pcballoc(struct socket *so, void *v)
{
	struct in6pcb *in6p;

	in6_pcbstate(in6p, IN6P_ATTACHED);
}

int
in6_pcbbind(v, nam, p)
{
	struct sockaddr_in6 *sin6;
	in6_pcblookup_port(table, &zeroin6_addr, 0, &sin6->sin6_addr, lport, wild);
}

struct in6pcb *
in6_pcblookup_port(struct inpcbtable *table, struct in6_addr *faddr6, u_int fport_arg, struct in6_addr *laddr6, u_int lport_arg, int lookup_wildcard)
{
	u_int16_t fport = fport_arg, lport = lport_arg;

	return (in6_pcblookup_local(table, faddr6, fport, laddr6, lport, lookup_wildcard));
}

struct in6pcb *
in6_pcblookup_connect(struct inpcbtable *table, struct in6_addr *faddr6, u_int fport_arg, struct in6_addr *laddr6, u_int lport_arg, int faith)
{
	u_int16_t fport = fport_arg, lport = lport_arg;

	return (in6_pcblookup_foreign(table, faddr6, fport, laddr6, lport, faith));
}

struct in6pcb *
in6_pcblookup_bind(struct inpcbtable *table, struct in6_addr *faddr6, u_int fport_arg, struct in6_addr *laddr6, u_int lport_arg, int faith)
{
	struct in6pcb *in6p;
	u_int16_t fport = fport_arg, lport = lport_arg;
#ifdef INET
	struct in6_addr zero_mapped;
#endif

	in6p = in6_pcblookup_foreign(table, faddr6, fport, laddr6, lport, faith);
	if (in6p != NULL) {
		return (in6p);
	}
#ifdef INET
	if (IN6_IS_ADDR_V4MAPPED(laddr6)) {
		memset(&zero_mapped, 0, sizeof(zero_mapped));
		zero_mapped.s6_addr16[5] = 0xffff;
		in6p = in6_pcblookup_foreign(table, faddr6, fport, &zero_mapped, lport, faith);
		if (in6p != NULL) {
			return (in6p);
		}
	}
#endif
	in6p = in6_pcblookup_foreign(table, faddr6, fport, &zeroin6_addr, lport, faith);
	if (in6p != NULL) {
		return (in6p);
	}
	return (NULL);
}

struct in6pcb *
in6_pcblookup_local(struct inpcbtable *table, struct in6_addr *faddr6, u_int fport_arg, struct in6_addr *laddr6, u_int lport_arg, int lookup_wildcard)
{
	u_int16_t fport = fport_arg, lport = lport_arg;

	return (in6_pcbhashlookup(table, faddr6, fport, laddr6, lport, 1, lookup_wildcard, IN6PLOOKUP_LOCAL));
}

struct in6pcb *
in6_pcblookup_foreign(struct inpcbtable *table, struct in6_addr *faddr6, u_int fport_arg, struct in6_addr *laddr6, u_int lport_arg, int faith)
{
	u_int16_t fport = fport_arg, lport = lport_arg;

	return (in6_pcbhashlookup(table, faddr6, fport, laddr6, lport, faith, 1, IN6PLOOKUP_FOREIGN));
}

static struct in6pcb *
in6_pcbhashlookup(struct inpcbtable *table, struct in6_addr *faddr6, u_int fport_arg, struct in6_addr *laddr6, u_int lport_arg, int faith, int lookup_wildcard, int which)
{
	struct inpcbhead *head;
	struct inpcb_hdr *inph;
	struct in6pcb *in6p, *match;
	int matchwild = 3, wildcard;
	u_int16_t fport = fport_arg, lport = lport_arg;

	switch (which) {
	case IN6PLOOKUP_FOREIGN:
		head = IN6PCBHASH_FOREIGN(table, laddr6, lport, faddr6, fport);
		LIST_FOREACH(inph, head, inph_fhash) {
			in6p = (struct in6pcb*) inph;
			if (in6p->in6p_af != AF_INET6) {
				continue;
			}
			if (faith && (in6p->in6p_flags & IN6P_FAITH) == 0) {
				continue;
			}
			if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
				continue;
			}
			if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr)) {
				continue;
			}
			if ((IN6_IS_ADDR_V4MAPPED(laddr6) || IN6_IS_ADDR_V4MAPPED(faddr6))
					&& (in6p->in6p_flags & IN6P_IPV6_V6ONLY)) {
				continue;
			}
			if (IN6_ARE_ADDR_EQUAL(in6p->in6p_faddr, faddr6)
					&& in6p->in6p_fport == fport && in6p->in6p_lport == lport
					&& IN6_ARE_ADDR_EQUAL(in6p->in6p_laddr, laddr6)) {
				goto out;
			}
		}
		break;

	case IN6PLOOKUP_LOCAL:
		head = IN6PCBHASH_LOCAL(table, laddr6, lport, faddr6, fport);
		LIST_FOREACH(inph, head, inph_lhash) {
			if (in6p->in6p_af != AF_INET6) {
				continue;
			}
			if (in6p->in6p_lport != lport) {
				continue;
			}
			wildcard = 0;
			if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_faddr)) {
				if ((in6p->in6p_flags & IN6P_IPV6_V6ONLY) != 0) {
					continue;
				}
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
				wildcard++;
			}
			if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr)) {
				if ((in6p->in6p_flags & IN6P_IPV6_V6ONLY) != 0) {
					continue;
				}
				if (!IN6_IS_ADDR_V4MAPPED(laddr6)) {
					continue;
				}

				/* duplicate of IPv4 logic */
				wildcard = 0;
				if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_faddr)
						&& in6p->in6p_faddr.s6_addr32[3]) {
					wildcard++;
				}
				if (!in6p->in6p_laddr.s6_addr32[3]) {
					if (laddr6->s6_addr32[3]) {
						wildcard++;
					}
				} else {
					if (!laddr6->s6_addr32[3]) {
						wildcard++;
					} else {
						if (in6p->in6p_laddr.s6_addr32[3]
								!= laddr6->s6_addr32[3]) {
							continue;
						}
					}
				}
			} else if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr)) {
				if (IN6_IS_ADDR_V4MAPPED(laddr6)) {
					if ((in6p->in6p_flags & IN6P_IPV6_V6ONLY) != 0) {
						continue;
					}
				}
				if (!IN6_IS_ADDR_UNSPECIFIED(laddr6)) {
					wildcard++;
				}
			} else {
				if (IN6_IS_ADDR_V4MAPPED(laddr6)) {
					if ((in6p->in6p_flags & IN6P_IPV6_V6ONLY) != 0)
						continue;
				}
				if (IN6_IS_ADDR_UNSPECIFIED(laddr6)) {
					wildcard++;
				} else {
					if (!IN6_ARE_ADDR_EQUAL(&in6p->in6p_laddr, laddr6)) {
						continue;
					}
				}
			}
			if (wildcard && !lookup_wildcard) {
				continue;
			}
			if (wildcard < matchwild) {
				match = in6p;
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

	return (NULL);

out:
	in6_pcbrehash(head, in6p, which);
	return (in6p);

found:
	return (match);
}

static void
in6_pcbrehash(struct inpcbhead *head, struct in6pcb *in6p, int which)
{
	struct inpcb_hdr *inph;

	inph = &in6p->in6p_head;
	if (inph != LIST_FIRST(head)) {
		switch (which) {
		case IN6PLOOKUP_FOREIGN:
			LIST_REMOVE(inph, inph_fhash);
			LIST_INSERT_HEAD(head, inph, inph_fhash);
			break;
		case IN6PLOOKUP_LOCAL:
			LIST_REMOVE(inph, inph_lhash);
			LIST_INSERT_HEAD(head, inph, inph_lhash);
			break;
		}
	}
}

static void
in6_pcbinsert(struct inpcbtable *table, struct in6pcb *in6p, int which)
{
	switch (which) {
	case IN6PLOOKUP_FOREIGN:
		LIST_INSERT_HEAD(
				IN6PCBHASH_FOREIGN(table, &in6p->in6p_faddr, in6p->in6p_fport, &in6p->in6p_laddr, in6p->in6p_lport),
				&in6p->in6p_head, inph_fhash);
		break;
	case IN6PLOOKUP_LOCAL:
		LIST_INSERT_HEAD(
				IN6PCBHASH_LOCAL(table, &in6p->in6p_faddr, in6p->in6p_fport, &in6p->in6p_laddr, in6p->in6p_lport),
				&in6p->in6p_head, inph_lhash);
		break;
	}
}

static void
in6_pcbremove(struct in6pcb *in6p, int which)
{
	switch (which) {
	case IN6PLOOKUP_FOREIGN:
		LIST_REMOVE(&in6p->in6p_head, inph_fhash);
		break;
	case IN6PLOOKUP_LOCAL:
		LIST_REMOVE(&in6p->in6p_head, inph_lhash);
		break;
	}
}
