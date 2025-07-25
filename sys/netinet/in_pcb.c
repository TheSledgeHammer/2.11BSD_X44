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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in_pcb.c,v 1.94 2004/03/02 02:26:28 thorpej Exp $");

#include "opt_inet.h"
#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#endif

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#elif FAST_IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif /* IPSEC */

struct in_addr zeroin_addr;

#define	INPCBHASH_PORT(table, lport) \
	&(table)->inpt_porthashtbl[ntohs(lport) & (table)->inpt_porthash]
#define	INPCBHASH_BIND(table, laddr, lport) \
	&(table)->inpt_bindhashtbl[ \
	    ((ntohl((laddr).s_addr) + ntohs(lport))) & (table)->inpt_bindhash]
#define	INPCBHASH_CONNECT(table, faddr, fport, laddr, lport) \
	&(table)->inpt_connecthashtbl[ \
	    ((ntohl((faddr).s_addr) + ntohs(fport)) + \
	     (ntohl((laddr).s_addr) + ntohs(lport))) & (table)->inpt_connecthash]

int	anonportmin = IPPORT_ANONMIN;
int	anonportmax = IPPORT_ANONMAX;
int	lowportmin  = IPPORT_RESERVEDMIN;
int	lowportmax  = IPPORT_RESERVEDMAX;

void
in_pcbinit(table, bindhashsize, connecthashsize)
	struct inpcbtable *table;
	int bindhashsize, connecthashsize;
{
	CIRCLEQ_INIT(&table->inpt_queue);
	table->inpt_porthashtbl = hashinit(bindhashsize, M_PCB, &table->inpt_porthash);
	table->inpt_bindhashtbl = hashinit(bindhashsize, M_PCB, &table->inpt_bindhash);
	table->inpt_connecthashtbl = hashinit(connecthashsize, M_PCB, &table->inpt_connecthash);
	table->inpt_lastlow = IPPORT_RESERVEDMAX;
	table->inpt_lastport = (u_int16_t)anonportmax;
}

int
in_pcballoc(so, v)
	struct socket *so;
	void *v;
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
	CIRCLEQ_INSERT_HEAD(&table->inpt_queue, &inp->inp_head,
	    inph_queue);
	LIST_INSERT_HEAD(INPCBHASH_PORT(table, inp->inp_lport), &inp->inp_head,
	    inph_lhash);
	in_pcbstate(inp, INP_ATTACHED);
	splx(s);
	return (0);
}

int
in_pcbbind(v, nam, p)
	void *v;
	struct mbuf *nam;
	struct proc *p;
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
	if ((so->so_options & (SO_REUSEADDR|SO_REUSEPORT)) == 0)
		wild = 1;
	if (nam == 0)
		goto noname;
	sin = mtod(nam, struct sockaddr_in *);
	if (nam->m_len != sizeof (*sin))
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
			reuseport = SO_REUSEADDR|SO_REUSEPORT;
	} else if (!in_nullhost(sin->sin_addr)) {
		sin->sin_port = 0;		/* yech... */
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
		if (ntohs(lport) < IPPORT_RESERVED &&
		    (p == 0 || suser1(p->p_ucred, &p->p_acflag)))
			return (EACCES);
#endif
#ifdef INET6
		memset(&mapped, 0, sizeof(mapped));
		mapped.s6_addr16[5] = 0xffff;
		memcpy(&mapped.s6_addr32[3], &sin->sin_addr,
		    sizeof(mapped.s6_addr32[3]));
		t6 = in6_pcblookup_port(table, &mapped, lport, wild);
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
			if (t &&
			    (!in_nullhost(sin->sin_addr) ||
			     !in_nullhost(t->inp_laddr) ||
			     (t->inp_socket->so_options & SO_REUSEPORT) == 0)
			    && (so->so_uid != t->inp_socket->so_uid)) {
				return (EADDRINUSE);
			}
		}
		t = in_pcblookup_port(table, sin->sin_addr, lport, wild);
		if (t && (reuseport & t->inp_socket->so_options) == 0)
			return (EADDRINUSE);
	}
	inp->inp_laddr = sin->sin_addr;

noname:
	if (lport == 0) {
		int	   cnt;
		u_int16_t  min, max;
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
		if (min > max) {	/* sanity check */
			u_int16_t swp;

			swp = min;
			min = max;
			max = swp;
		}

		lport = *lastport - 1;
		for (cnt = max - min + 1; cnt; cnt--, lport--) {
			if (lport < min || lport > max)
				lport = max;
			if (!in_pcblookup_port(table, inp->inp_laddr,
			    htons(lport), 1))
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
	LIST_REMOVE(&inp->inp_head, inph_lhash);
	LIST_INSERT_HEAD(INPCBHASH_PORT(table, inp->inp_lport), &inp->inp_head,
	    inph_lhash);
	in_pcbstate(inp, INP_BOUND);
	return (0);
}

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
int
in_pcbconnect(v, nam)
	void *v;
	struct mbuf *nam;
{
	struct inpcb *inp = v;
	struct in_ifaddr *ia = NULL;
	struct sockaddr_in *ifaddr = NULL;
	struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
	int error;

	if (inp->inp_af != AF_INET)
		return (EINVAL);

	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	if (sin->sin_port == 0)
		return (EADDRNOTAVAIL);
	if (TAILQ_FIRST(&in_ifaddrhead) != 0) {
		/*
		 * If the destination address is INADDR_ANY,
		 * use any local address (likely loopback).
		 * If the supplied address is INADDR_BROADCAST,
		 * use the broadcast address of an interface
		 * which supports broadcast. (loopback does not)
		 */

		if (in_nullhost(sin->sin_addr)) {
			sin->sin_addr =
			    TAILQ_FIRST(&in_ifaddrhead)->ia_addr.sin_addr;
		} else if (sin->sin_addr.s_addr == INADDR_BROADCAST) {
			TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list) {
				if (ia->ia_ifp->if_flags & IFF_BROADCAST) {
					sin->sin_addr =
					    ia->ia_broadaddr.sin_addr;
					break;
				}
			}
		}
	}
	/*
	 * If we haven't bound which network number to use as ours,
	 * we will use the number of the outgoing interface.
	 * This depends on having done a routing lookup, which
	 * we will probably have to do anyway, so we might
	 * as well do it now.  On the other hand if we are
	 * sending to multiple destinations we may have already
	 * done the lookup, so see if we can use the route
	 * from before.  In any case, we only
	 * chose a port number once, even if sending to multiple
	 * destinations.
	 */
	if (in_nullhost(inp->inp_laddr)) {
		int error;
		ifaddr = in_selectsrc(sin, &inp->inp_route,
			inp->inp_socket->so_options, inp->inp_moptions, &error);
		if (ifaddr == NULL) {
			if (error == 0)
				error = EADDRNOTAVAIL;
			return error;
		}
		INADDR_TO_IA(ifaddr->sin_addr, ia);
		if (ia == NULL)
			return (EADDRNOTAVAIL);
	}
	if (in_pcblookup_connect(inp->inp_table, sin->sin_addr, sin->sin_port,
	    !in_nullhost(inp->inp_laddr) ? inp->inp_laddr : ifaddr->sin_addr,
	    inp->inp_lport) != 0)
		return (EADDRINUSE);
	if (in_nullhost(inp->inp_laddr)) {
		if (inp->inp_lport == 0) {
			error = in_pcbbind(inp, (struct mbuf *)0,
			    (struct proc *)0);
			/*
			 * This used to ignore the return value
			 * completely, but we need to check for
			 * ephemeral port shortage.
			 * XXX Should we check for other errors, too?
			 */
			if (error == EAGAIN)
				return (error);
		}
		inp->inp_laddr = ia->ia_addr.sin_addr;
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	in_pcbstate(inp, INP_CONNECTED);
#if defined(IPSEC) || defined(FAST_IPSEC)
	if (inp->inp_socket->so_type == SOCK_STREAM)
		ipsec_pcbconn(inp->inp_sp);
#endif
	return (0);
}

void
in_pcbdisconnect(v)
	void *v;
{
	struct inpcb *inp = v;

	if (inp->inp_af != AF_INET)
		return;

	inp->inp_faddr = zeroin_addr;
	inp->inp_fport = 0;
	in_pcbstate(inp, INP_BOUND);
#if defined(IPSEC) || defined(FAST_IPSEC)
	ipsec_pcbdisconn(inp->inp_sp);
#endif
	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
}

void
in_pcbdetach(v)
	void *v;
{
	struct inpcb *inp = v;
	struct socket *so = inp->inp_socket;
	int s;

	if (inp->inp_af != AF_INET)
		return;

#if defined(IPSEC) || defined(FAST_IPSEC)
	ipsec4_delete_pcbpolicy(inp);
#endif /*IPSEC*/
	so->so_pcb = 0;
	sofree(so);
	if (inp->inp_options)
		(void)m_free(inp->inp_options);
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt);
	ip_freemoptions(inp->inp_moptions);
	s = splnet();
	in_pcbstate(inp, INP_ATTACHED);
	LIST_REMOVE(&inp->inp_head, inph_lhash);
	CIRCLEQ_REMOVE(&inp->inp_table->inpt_queue, &inp->inp_head,
	    inph_queue);
	splx(s);
	FREE(inp, M_PCB);
}

void
in_setsockaddr(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in *sin;

	if (inp->inp_af != AF_INET)
		return;

	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

void
in_setpeeraddr(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in *sin;

	if (inp->inp_af != AF_INET)
		return;

	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  The local address and/or port numbers
 * may be specified to limit the search.  The "usual action" will be
 * taken, depending on the ctlinput cmd.  The caller must filter any
 * cmds that are uninteresting (e.g., no error in the map).
 * Call the protocol specific routine (if any) to report
 * any errors for each matching socket.
 *
 * Must be called at splsoftnet.
 */
int
in_pcbnotify(table, faddr, fport_arg, laddr, lport_arg, errno, notify)
	struct inpcbtable *table;
	struct in_addr faddr, laddr;
	u_int fport_arg, lport_arg;
	int errno;
	void (*notify)(struct inpcb *, int);
{
	struct inpcbhead *head;
	struct inpcb *inp, *ninp;
	u_int16_t fport = fport_arg, lport = lport_arg;
	int nmatch;

	if (in_nullhost(faddr) || notify == 0)
		return (0);

	nmatch = 0;
	head = INPCBHASH_CONNECT(table, faddr, fport, laddr, lport);
	for (inp = (struct inpcb *)LIST_FIRST(head); inp != NULL; inp = ninp) {
		ninp = (struct inpcb *)LIST_NEXT(inp, inp_hash);
		if (inp->inp_af != AF_INET)
			continue;
		if (in_hosteq(inp->inp_faddr, faddr) &&
		    inp->inp_fport == fport &&
		    inp->inp_lport == lport &&
		    in_hosteq(inp->inp_laddr, laddr)) {
			(*notify)(inp, errno);
			nmatch++;
		}
	}
	return (nmatch);
}

void
in_pcbnotifyall(table, faddr, errno, notify)
	struct inpcbtable *table;
	struct in_addr faddr;
	int errno;
	void (*notify)(struct inpcb *, int);
{
	struct inpcb *inp, *ninp;

	if (in_nullhost(faddr) || notify == 0)
		return;

	for (inp = (struct inpcb *)CIRCLEQ_FIRST(&table->inpt_queue);
	    inp != (void *)&table->inpt_queue;
	    inp = ninp) {
		ninp = (struct inpcb *)CIRCLEQ_NEXT(inp, inp_queue);
		if (inp->inp_af != AF_INET)
			continue;
		if (in_hosteq(inp->inp_faddr, faddr))
			(*notify)(inp, errno);
	}
}

void
in_pcbpurgeif0(table, ifp)
	struct inpcbtable *table;
	struct ifnet *ifp;
{
	struct inpcb *inp, *ninp;
	struct ip_moptions *imo;
	int i, gap;

	for (inp = (struct inpcb *)CIRCLEQ_FIRST(&table->inpt_queue);
	    inp != (void *)&table->inpt_queue;
	    inp = ninp) {
		ninp = (struct inpcb *)CIRCLEQ_NEXT(inp, inp_queue);
		if (inp->inp_af != AF_INET)
			continue;
		imo = inp->inp_moptions;
		if (imo != NULL) {
			/*
			 * Unselect the outgoing interface if it is being
			 * detached.
			 */
			if (imo->imo_multicast_ifp == ifp)
				imo->imo_multicast_ifp = NULL;

			/*
			 * Drop multicast group membership if we joined
			 * through the interface being detached.
			 */
			for (i = 0, gap = 0; i < imo->imo_num_memberships;
			    i++) {
				if (imo->imo_membership[i]->inm_ifp == ifp) {
					in_delmulti(imo->imo_membership[i]);
					gap++;
				} else if (gap != 0)
					imo->imo_membership[i - gap] =
					    imo->imo_membership[i];
			}
			imo->imo_num_memberships -= gap;
		}
	}
}

void
in_pcbpurgeif(table, ifp)
	struct inpcbtable *table;
	struct ifnet *ifp;
{
	struct inpcb *inp, *ninp;

	for (inp = (struct inpcb *)CIRCLEQ_FIRST(&table->inpt_queue);
	    inp != (void *)&table->inpt_queue;
	    inp = ninp) {
		ninp = (struct inpcb *)CIRCLEQ_NEXT(inp, inp_queue);
		if (inp->inp_af != AF_INET)
			continue;
		if (inp->inp_route.ro_rt != NULL &&
		    inp->inp_route.ro_rt->rt_ifp == ifp)
			in_rtchange(inp, 0);
	}
}

/*
 * Check for alternatives when higher level complains
 * about service problems.  For now, invalidate cached
 * routing information.  If the route was created dynamically
 * (by a redirect), time to try a default gateway again.
 */
void
in_losing(inp)
	struct inpcb *inp;
{
	struct rtentry *rt;
	struct rt_addrinfo info;

	if (inp->inp_af != AF_INET)
		return;

	if ((rt = inp->inp_route.ro_rt)) {
		inp->inp_route.ro_rt = 0;
		bzero((caddr_t)&info, sizeof(info));
		info.rti_info[RTAX_DST] = &inp->inp_route.ro_dst;
		info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
		info.rti_info[RTAX_NETMASK] = rt_mask(rt);
		rt_missmsg(RTM_LOSING, &info, rt->rt_flags, 0);
		if (rt->rt_flags & RTF_DYNAMIC)
			(void) rtrequest(RTM_DELETE, rt_key(rt),
				rt->rt_gateway, rt_mask(rt), rt->rt_flags,
				(struct rtentry **)0);
		else
		/*
		 * A new route can be allocated
		 * the next time output is attempted.
		 */
			rtfree(rt);
	}
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
void
in_rtchange(inp, errno)
	struct inpcb *inp;
	int errno;
{

	if (inp->inp_af != AF_INET)
		return;

	if (inp->inp_route.ro_rt) {
		rtfree(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
	/* XXX SHOULD NOTIFY HIGHER-LEVEL PROTOCOLS */
}

struct inpcb *
in_pcblookup_port(table, laddr, lport_arg, lookup_wildcard)
	struct inpcbtable *table;
	struct in_addr laddr;
	u_int lport_arg;
	int lookup_wildcard;
{
	struct inpcbhead *head;
	struct inpcb_hdr *inph;
	struct inpcb *inp, *match = 0;
	int matchwild = 3, wildcard;
	u_int16_t lport = lport_arg;

	head = INPCBHASH_PORT(table, lport);
	LIST_FOREACH(inph, head, inph_lhash) {
		inp = (struct inpcb *)inph;
		if (inp->inp_af != AF_INET)
			continue;

		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (!in_nullhost(inp->inp_faddr))
			wildcard++;
		if (in_nullhost(inp->inp_laddr)) {
			if (!in_nullhost(laddr))
				wildcard++;
		} else {
			if (in_nullhost(laddr))
				wildcard++;
			else {
				if (!in_hosteq(inp->inp_laddr, laddr))
					continue;
			}
		}
		if (wildcard && !lookup_wildcard)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}
	return (match);
}

#ifdef DIAGNOSTIC
int	in_pcbnotifymiss = 0;
#endif

struct inpcb *
in_pcblookup_connect(table, faddr, fport_arg, laddr, lport_arg)
	struct inpcbtable *table;
	struct in_addr faddr, laddr;
	u_int fport_arg, lport_arg;
{
	struct inpcbhead *head;
	struct inpcb_hdr *inph;
	struct inpcb *inp;
	u_int16_t fport = fport_arg, lport = lport_arg;

	head = INPCBHASH_CONNECT(table, faddr, fport, laddr, lport);
	LIST_FOREACH(inph, head, inph_hash) {
		inp = (struct inpcb *)inph;
		if (inp->inp_af != AF_INET)
			continue;

		if (in_hosteq(inp->inp_faddr, faddr) &&
		    inp->inp_fport == fport &&
		    inp->inp_lport == lport &&
		    in_hosteq(inp->inp_laddr, laddr))
			goto out;
	}
#ifdef DIAGNOSTIC
	if (in_pcbnotifymiss) {
		printf("in_pcblookup_connect: faddr=%08x fport=%d laddr=%08x lport=%d\n",
		    ntohl(faddr.s_addr), ntohs(fport),
		    ntohl(laddr.s_addr), ntohs(lport));
	}
#endif
	return (0);

out:
	/* Move this PCB to the head of hash chain. */
	inph = &inp->inp_head;
	if (inph != LIST_FIRST(head)) {
		LIST_REMOVE(inph, inph_hash);
		LIST_INSERT_HEAD(head, inph, inph_hash);
	}
	return (inp);
}

struct inpcb *
in_pcblookup_bind(table, laddr, lport_arg)
	struct inpcbtable *table;
	struct in_addr laddr;
	u_int lport_arg;
{
	struct inpcbhead *head;
	struct inpcb_hdr *inph;
	struct inpcb *inp;
	u_int16_t lport = lport_arg;

	head = INPCBHASH_BIND(table, laddr, lport);
	LIST_FOREACH(inph, head, inph_hash) {
		inp = (struct inpcb *)inph;
		if (inp->inp_af != AF_INET)
			continue;

		if (inp->inp_lport == lport &&
		    in_hosteq(inp->inp_laddr, laddr))
			goto out;
	}
	head = INPCBHASH_BIND(table, zeroin_addr, lport);
	LIST_FOREACH(inph, head, inph_hash) {
		inp = (struct inpcb *)inph;
		if (inp->inp_af != AF_INET)
			continue;

		if (inp->inp_lport == lport &&
		    in_hosteq(inp->inp_laddr, zeroin_addr))
			goto out;
	}
#ifdef DIAGNOSTIC
	if (in_pcbnotifymiss) {
		printf("in_pcblookup_bind: laddr=%08x lport=%d\n",
		    ntohl(laddr.s_addr), ntohs(lport));
	}
#endif
	return (0);

out:
	/* Move this PCB to the head of hash chain. */
	inph = &inp->inp_head;
	if (inph != LIST_FIRST(head)) {
		LIST_REMOVE(inph, inph_hash);
		LIST_INSERT_HEAD(head, inph, inph_hash);
	}
	return (inp);
}

void
in_pcbstate(inp, state)
	struct inpcb *inp;
	int state;
{

	if (inp->inp_af != AF_INET)
		return;

	if (inp->inp_state > INP_ATTACHED)
		LIST_REMOVE(&inp->inp_head, inph_hash);

	switch (state) {
	case INP_BOUND:
		LIST_INSERT_HEAD(INPCBHASH_BIND(inp->inp_table,
		    inp->inp_laddr, inp->inp_lport), &inp->inp_head,
		    inph_hash);
		break;
	case INP_CONNECTED:
		LIST_INSERT_HEAD(INPCBHASH_CONNECT(inp->inp_table,
		    inp->inp_faddr, inp->inp_fport,
		    inp->inp_laddr, inp->inp_lport), &inp->inp_head,
		    inph_hash);
		break;
	}

	inp->inp_state = state;
}

struct rtentry *
in_pcbrtentry(inp)
	struct inpcb *inp;
{
	struct route *ro;

	if (inp->inp_af != AF_INET)
		return (NULL);

	ro = &inp->inp_route;

	if (ro->ro_rt && ((ro->ro_rt->rt_flags & RTF_UP) == 0 ||
	    !in_hosteq(satosin(&ro->ro_dst)->sin_addr, inp->inp_faddr))) {
		RTFREE(ro->ro_rt);
		ro->ro_rt = (struct rtentry *)NULL;
	}
	if (ro->ro_rt == (struct rtentry *)NULL &&
	    !in_nullhost(inp->inp_faddr)) {
		bzero(&ro->ro_dst, sizeof(struct sockaddr_in));
		ro->ro_dst.sa_family = AF_INET;
		ro->ro_dst.sa_len = sizeof(ro->ro_dst);
		satosin(&ro->ro_dst)->sin_addr = inp->inp_faddr;
		rtalloc(ro);
	}
	return (ro->ro_rt);
}

struct sockaddr_in *
in_selectsrc(sin, ro, soopts, mopts, errorp)
	struct sockaddr_in *sin;
	struct route *ro;
	int soopts;
	struct ip_moptions *mopts;
	int *errorp;
{
	struct in_ifaddr *ia;

	ia = (struct in_ifaddr *)0;
	/*
	 * If route is known or can be allocated now,
	 * our src addr is taken from the i/f, else punt.
	 * Note that we should check the address family of the cached
	 * destination, in case of sharing the cache with IPv6.
	 */
	if (ro->ro_rt &&
	    (ro->ro_dst.sa_family != AF_INET ||
	    !in_hosteq(satosin(&ro->ro_dst)->sin_addr, sin->sin_addr) ||
	    (soopts & SO_DONTROUTE))) {
		RTFREE(ro->ro_rt);
		ro->ro_rt = (struct rtentry *)0;
	}
	if ((soopts & SO_DONTROUTE) == 0 && /*XXX*/
	    (ro->ro_rt == (struct rtentry *)0 ||
	     ro->ro_rt->rt_ifp == (struct ifnet *)0)) {
		/* No route yet, so try to acquire one */
		bzero(&ro->ro_dst, sizeof(struct sockaddr_in));
		ro->ro_dst.sa_family = AF_INET;
		ro->ro_dst.sa_len = sizeof(struct sockaddr_in);
		satosin(&ro->ro_dst)->sin_addr = sin->sin_addr;
		rtalloc(ro);
	}
	/*
	 * If we found a route, use the address
	 * corresponding to the outgoing interface
	 * unless it is the loopback (in case a route
	 * to our address on another net goes to loopback).
	 *
	 * XXX Is this still true?  Do we care?
	 */
	if (ro->ro_rt && !(ro->ro_rt->rt_ifp->if_flags & IFF_LOOPBACK))
		ia = ifatoia(ro->ro_rt->rt_ifa);
	if (ia == NULL) {
		u_int16_t fport = sin->sin_port;

		sin->sin_port = 0;
		ia = ifatoia(ifa_ifwithladdr(sintosa(sin)));
		sin->sin_port = fport;
		if (ia == 0) {
			/* Find 1st non-loopback AF_INET address */
			TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list) {
				if (!(ia->ia_ifp->if_flags & IFF_LOOPBACK))
					break;
			}
		}
		if (ia == NULL) {
			*errorp = EADDRNOTAVAIL;
			return NULL;
		}
	}
	/*
	 * If the destination address is multicast and an outgoing
	 * interface has been set as a multicast option, use the
	 * address of that interface as our source address.
	 */
	if (IN_MULTICAST(sin->sin_addr.s_addr) && mopts != NULL) {
		struct ip_moptions *imo;
		struct ifnet *ifp;

		imo = mopts;
		if (imo->imo_multicast_ifp != NULL) {
			ifp = imo->imo_multicast_ifp;
			IFP_TO_IA(ifp, ia);		/* XXX */
			if (ia == 0) {
				*errorp = EADDRNOTAVAIL;
				return NULL;
			}
		}
	}
	return satosin(&ia->ia_addr);
}

/* Contemplating: Reverting to a single pcblookup */
#ifdef notyet
#include <sys/fnv_hash.h>

struct inpcbhead *
in_pcbhash(struct inpcbtable *table, struct in_addr laddr, u_int lport, struct in_addr faddr, u_int fport)
{
	struct inpcbhead *inhash;
	uint32_t nethash;

	uint32_t lahash = fnva_32_buf(laddr, sizeof(*laddr), FNV1_32_INIT);	/* laddr hash */
	uint32_t lphash = fnva_32_buf(&lport, sizeof(lport), FNV1_32_INIT);	/* lport hash */
	uint32_t fahash = fnva_32_buf(faddr, sizeof(*faddr), FNV1_32_INIT);	/* faddr hash */
	uint32_t fphash = fnva_32_buf(&fport, sizeof(fport), FNV1_32_INIT);	/* fport hash */

	nethash = ntohl(laddr.s_addr) + ntohs(lport) + ntohl(faddr.s_addr) + ntohs(fport);
	inhash = &table->inpt_connecthashtbl[nethash & table->inpt_connecthash];
	return (inhash);
}

struct inpcb *
in_pcblookup(table, laddr, lport_arg, faddr, fport_arg, lookup_wildcard)
	struct inpcbtable *table;
	struct in_addr laddr, faddr;
	u_int lport_arg, fport_arg;
	int lookup_wildcard;
{
	struct inpcbhead *head;
	struct inpcb_hdr *inph;
	struct inpcb *inp, *match;
	int matchwild = 3, wildcard;
	u_int16_t lport = lport_arg;
	u_int16_t fport = fport_arg;

	head = in_pcbhash(table, laddr, lport, faddr, fport);
	LIST_FOREACH(inph, head, inph_lhash) {
		inp = (struct inpcb*) inph;
		if (inp->inp_af != AF_INET) {
			continue;
		}
		if (inp->inp_lport != lport) {
			continue;
		}
		if (inp->inp_lport == lport && in_hosteq(inp->inp_laddr, laddr)) {
			goto out;
		}
		if (in_hosteq(inp->inp_faddr, faddr) && inp->inp_fport == fport
				&& inp->inp_lport == lport
				&& in_hosteq(inp->inp_laddr, laddr)) {
			goto out;
		}
		wildcard = 0;
		if (!in_nullhost(inp->inp_faddr)) {
			wildcard++;
		}
		if (in_nullhost(inp->inp_laddr)) {
			if (!in_nullhost(laddr)) {
				wildcard++;
			}
		} else {
			if (in_nullhost(laddr)) {
				wildcard++;
			} else {
				if (!in_hosteq(inp->inp_laddr, laddr)) {
					continue;
				}
			}
		}
		if (wildcard && !lookup_wildcard) {
			continue;
		}
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0) {
				goto match;
				//break;
			}
			goto match;
		}
	}
	head = in_pcbhash(table, zeroin_addr, lport, faddr, fport);
	LIST_FOREACH(inph, head, inph_hash) {
		inp = (struct inpcb *)inph;
		if (inp->inp_af != AF_INET) {
			continue;
		}
		if (inp->inp_lport == lport && in_hosteq(inp->inp_laddr, zeroin_addr)) {
			goto out;
		}
	}
#ifdef DIAGNOSTIC
	if (in_pcbnotifymiss) {
		printf("in_pcblookup: laddr=%08x lport=%d\n",
		    ntohl(laddr.s_addr), ntohs(lport));
	}
#endif
	return (0);

out:
	/* Move this PCB to the head of hash chain. */
	inph = &inp->inp_head;
	if (inph != LIST_FIRST(head)) {
		LIST_REMOVE(inph, inph_hash);
		LIST_INSERT_HEAD(head, inph, inph_hash);
	}
	return (inp);

match:
	return (match);
}
#endif /* notyet */
