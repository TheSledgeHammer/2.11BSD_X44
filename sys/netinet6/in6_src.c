/*	$NetBSD: in6_src.c,v 1.18 2003/12/10 11:46:33 itojun Exp $	*/
/*	$KAME: in6_src.c,v 1.163 2007/06/14 13:51:34 itojun Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in6_src.c,v 1.18 2003/12/10 11:46:33 itojun Exp $");

#include "opt_inet.h"
#include "opt_radix.h"

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
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>

#include <net/net_osdep.h>

#include "loop.h"

extern struct ifnet loif[NLOOP];

static int selectroute(struct sockaddr_in6 *, struct ip6_pktopts *, struct route_in6 *, struct rtentry *, struct ifnet **);
static struct rtentry *in6_rtentry(struct ip6_pktopts *, struct ifnet **, int *);
static int in6_selectif(struct sockaddr_in6 *, struct ip6_pktopts *, struct route_in6 *, struct ifnet **);

/*
 * Return an IPv6 address, which is the most appropriate for a given
 * destination and user specified options.
 * If necessary, this function lookups the routing table and returns
 * an entry to the caller for later use.
 */
struct in6_addr *
in6_selectsrc(dstsock, opts, mopts, ro, laddr, errorp)
	struct sockaddr_in6 *dstsock;
	struct ip6_pktopts *opts;
	struct ip6_moptions *mopts;
	struct route_in6 *ro;
	struct in6_addr *laddr;
	int *errorp;
{
	struct ifnet *ifp = NULL;
	struct in6_addr *dst;
	struct in6_ifaddr *ia6 = 0;
	struct in6_pktinfo *pi = NULL;

	dst = &dstsock->sin6_addr;
	*errorp = 0;

	/*
	 * If the source address is explicitly specified by the caller,
	 * use it.
	 */
	if (opts && (pi = opts->ip6po_pktinfo) &&
	    !IN6_IS_ADDR_UNSPECIFIED(&pi->ipi6_addr)) {
		/* get the outgoing interface */
		if ((*errorp = in6_selectif(dstsock, opts, ro, &ifp)) != 0) {
			return (NULL);
		}
		if (ifp) {
			*errorp = in6_setscope(&pi->ipi6_addr, ifp, NULL);
			if (*errorp != 0) {
				return (NULL);
			}
		}
		ia6 = in6_ifawithscope(ifp, &pi->ipi6_addr);
		if (ia6 == NULL ||
		    (ia6->ia6_flags & (IN6_IFF_ANYCAST | IN6_IFF_NOTREADY))) {
			*errorp = EADDRNOTAVAIL;
			return (NULL);
		}
		return (&pi->ipi6_addr);
	}

	/*
	 * If the source address is not specified but the socket(if any)
	 * is already bound, use the bound address.
	 */
	if (laddr && !IN6_IS_ADDR_UNSPECIFIED(laddr)) {
		return (laddr);
	}

	/*
	 * If the caller doesn't specify the source address but
	 * the outgoing interface, use an address associated with
	 * the interface.
	 */
	if ((*errorp = in6_selectif(dstsock, opts, ro, &ifp)) != 0) {
		return (NULL);
	}
	if (pi && pi->ipi6_ifindex) {
		/* XXX boundary check is assumed to be already done. */
		ia6 = in6_ifawithscope(ifindex2ifnet[pi->ipi6_ifindex], dst);
		if (ia6 == NULL) {
			*errorp = EADDRNOTAVAIL;
			return (NULL);
		}
		return (&satosin6(&ia6->ia_addr)->sin6_addr);
	}

	/*
	 * If the destination address is a link-local unicast address or
	 * a multicast address, and if the outgoing interface is specified
	 * by the sin6_scope_id filed, use an address associated with the
	 * interface.
	 * XXX: We're now trying to define more specific semantics of
	 *      sin6_scope_id field, so this part will be rewritten in
	 *      the near future.
	 */
	if ((IN6_IS_ADDR_LINKLOCAL(dst) || IN6_IS_ADDR_MULTICAST(dst)) &&
	    dstsock->sin6_scope_id) {
		/*
		 * I'm not sure if boundary check for scope_id is done
		 * somewhere...
		 */
		if (dstsock->sin6_scope_id < 0 ||
		    if_indexlim <= dstsock->sin6_scope_id ||
		    !ifindex2ifnet[dstsock->sin6_scope_id]) {
			*errorp = ENXIO; /* XXX: better error? */
			return (0);
		}
		ia6 = in6_ifawithscope(ifindex2ifnet[dstsock->sin6_scope_id],
				       dst);
		if (ia6 == 0) {
			*errorp = EADDRNOTAVAIL;
			return (NULL);
		}
		return (&satosin6(&ia6->ia_addr)->sin6_addr);
	}

	/*
	 * If the destination address is a multicast address and
	 * the outgoing interface for the address is specified
	 * by the caller, use an address associated with the interface.
	 * There is a sanity check here; if the destination has node-local
	 * scope, the outgoing interfacde should be a loopback address.
	 * Even if the outgoing interface is not specified, we also
	 * choose a loopback interface as the outgoing interface.
	 */
	if (IN6_IS_ADDR_MULTICAST(dst)) {
		struct ifnet *ifp = mopts ? mopts->im6o_multicast_ifp : NULL;

		if (ifp == NULL && IN6_IS_ADDR_MC_NODELOCAL(dst)) {
			ifp = &loif[0];
		}

		if (ifp) {
			ia6 = in6_ifawithscope(ifp, dst);
			if (ia6 == NULL) {
				*errorp = EADDRNOTAVAIL;
				return (NULL);
			}
			return (&satosin6(&ia6->ia_addr)->sin6_addr);
		}
	}

	if (ro) {
		if ((*errorp = in6_selectif(dstsock, opts, ro, &ifp)) != 0) {
			return (NULL);
		}
		ia6 = in6_ifawithscope(ro->ro_rt->rt_ifa->ifa_ifp, dst);
		if (ia6 == NULL) { /* xxx scope error ?*/
			ia6 = ifatoia6(ro->ro_rt->rt_ifa);
		}
#if 0
		/*
		 * xxx The followings are necessary? (kazu)
		 * I don't think so.
		 * It's for SO_DONTROUTE option in IPv4.(jinmei)
		 */
		if (ia6 == 0) {
			struct sockaddr_in6 sin6 = {sizeof(sin6), AF_INET6, 0};

			sin6->sin6_addr = *dst;

			ia6 = ifatoia6(ifa_ifwithdstaddr(sin6tosa(&sin6)));
			if (ia6 == 0)
				ia6 = ifatoia6(ifa_ifwithnet(sin6tosa(&sin6)));
			if (ia6 == 0)
				return (0);
			return (&satosin6(&ia6->ia_addr)->sin6_addr);
		}
#endif /* 0 */
		if (ia6 == NULL) {
			*errorp = EHOSTUNREACH;	/* no route */
			return (NULL);
		}
		return (&satosin6(&ia6->ia_addr)->sin6_addr);
	}

	*errorp = EADDRNOTAVAIL;
	return (NULL);
}

static int
selectroute(dstsock, opts, ro, rt, retifp)
	struct sockaddr_in6 *dstsock;
	struct ip6_pktopts *opts;
	struct route_in6 *ro;
	struct rtentry *rt;
	struct ifnet **retifp;
{
	struct ifnet *ifp = NULL;
	struct in6_addr *dst;
	int error = 0;

	ifp = *retifp;
	dst = &dstsock->sin6_addr;

	/*
	 * If route is known or can be allocated now,
	 * our src addr is taken from the i/f, else punt.
	 * Note that we should check the address family of the
	 * cached destination, in case of sharing the cache with IPv4.
	 */
	if (ro) {
		if (ro->ro_rt &&
				(!(ro->ro_rt->rt_flags & RTF_UP) ||
				((struct sockaddr *)(&ro->ro_dst))->sa_family != AF_INET6 ||
				!IN6_ARE_ADDR_EQUAL(&satosin6(&ro->ro_dst)->sin6_addr,dst))) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)NULL;
		}
		if (ro->ro_rt == (struct rtentry *)NULL ||
		    ro->ro_rt->rt_ifp == (struct ifnet *)NULL) {
			struct sockaddr_in6 *sa6;

			/* No route yet, so try to acquire one */
			bzero(&ro->ro_dst, sizeof(struct sockaddr_in6));
			sa6 = (struct sockaddr_in6 *)&ro->ro_dst;
			sa6->sin6_family = AF_INET6;
			sa6->sin6_len = sizeof(struct sockaddr_in6);
			sa6->sin6_addr = *dst;
			sa6->sin6_scope_id = dstsock->sin6_scope_id;
			if (IN6_IS_ADDR_MULTICAST(dst)) {
#ifdef RADIX_MPATH
				rtalloc_mpath((struct route *)ro, ntohl(sa6->sin6_addr.s6_addr32[3]));
#else
				ro->ro_rt = rtalloc1(&((struct route *)ro)->ro_dst, 0);
#endif /* RADIX_MPATH */
			} else {
#ifdef RADIX_MPATH
				rtalloc_mpath((struct route *)ro, ntohl(sa6->sin6_addr.s6_addr32[3]));
#else
				rtalloc((struct route *)ro);
#endif /* RADIX_MPATH */
			}
		}
		/*
		 * do not care about the result if we have the nexthop
		 * explicitly specified.
		 */
		if (opts && opts->ip6po_nexthop) {
			goto done;
		}

		if (ro->ro_rt) {
			ifp = ro->ro_rt->rt_ifp;

			if (ifp == NULL) { /* can this really happen? */
				RTFREE(ro->ro_rt);
				ro->ro_rt = NULL;
			}
		}
		if (ro->ro_rt == NULL) {
			error = EHOSTUNREACH;
		}
		rt = ro->ro_rt;

		/*
		 * Check if the outgoing interface conflicts with
		 * the interface specified by ipi6_ifindex (if specified).
		 * Note that loopback interface is always okay.
		 * (this may happen when we are sending a packet to one of
		 *  our own addresses.)
		 */
		if (opts && opts->ip6po_pktinfo &&
		    opts->ip6po_pktinfo->ipi6_ifindex) {
			if (!(ifp->if_flags & IFF_LOOPBACK) &&
			    ifp->if_index !=
			    opts->ip6po_pktinfo->ipi6_ifindex) {
				error = EHOSTUNREACH;
				goto done;
			}
		}
	}

done:
	if (ifp == NULL && rt == NULL) {
		/*
		 * This can happen if the caller did not pass a cached route
		 * nor any other hints.  We treat this case an error.
		 */
		error = EHOSTUNREACH;
	}
	if (error == EHOSTUNREACH) {
		ip6stat.ip6s_noroute++;
	}
	if (retifp != NULL) {
		*retifp = ifp;
    }
	return (error);
}

static struct rtentry *
in6_rtentry(opts, retifp, errorp)
	struct ip6_pktopts *opts;
	struct ifnet **retifp;
	int *errorp;
{
	struct sockaddr_in6 *sin6_next;
	struct rtentry *rt;
	struct route_in6 *ron;

	/*
	 * If the next hop address for the packet is specified by the caller,
	 * use it as the gateway.
	 */
	if (opts && opts->ip6po_nexthop) {
		sin6_next = satosin6(opts->ip6po_nexthop);

		/* at this moment, we only support AF_INET6 next hops */
		if (sin6_next->sin6_family != AF_INET6) {
			*errorp = EAFNOSUPPORT;
			return (NULL);
		}

		/*
		 * If the next hop address for the packet is specified
		 * by caller, use an address associated with the route
		 * to the next hop.
		 */
		rt = nd6_lookup(&sin6_next->sin6_addr, 1, NULL);
		if (rt == NULL) {
			*errorp = EADDRNOTAVAIL;
			goto fail;
		}

		/*
		 * If the next hop is an IPv6 address, then the node identified
		 * by that address must be a neighbor of the sending host.
		 */
		ron = &opts->ip6po_nextroute;
		if ((ron->ro_rt
				&& (ron->ro_rt->rt_flags & (RTF_UP | RTF_GATEWAY)) !=
				RTF_UP) ||
				!IN6_ARE_ADDR_EQUAL(&satosin6(&ron->ro_dst)->sin6_addr,
						&sin6_next->sin6_addr)) {
			if (ron->ro_rt) {
				RTFREE(ron->ro_rt);
				ron->ro_rt = NULL;
			}
			*satosin6(&ron->ro_dst) = *sin6_next;
		}
		if (ron->ro_rt == NULL) {
			rtalloc((struct route *)ron); /* multi path case? */
			if (ron->ro_rt == NULL ||
			    (ron->ro_rt->rt_flags & RTF_GATEWAY)) {
				*errorp = EHOSTUNREACH;
				goto fail;
			}
		}
		if (!nd6_is_addr_neighbor(sin6_next, ron->ro_rt->rt_ifp)) {
			*errorp = EHOSTUNREACH;
			goto fail;
		}
	}

	if ((rt != NULL) && (ron->ro_rt != NULL)) {
		if (rt != ron->ro_rt) {
			rt = ron->ro_rt;
		}
	}
	if ((*retifp == NULL) || (*retifp != rt->rt_ifp)) {
		*retifp = rt->rt_ifp;
	}
	return (rt);
    
fail:
    if (rt) {
        RTFREE(rt);
        rt = NULL;
    }
    if (ron->ro_rt) {
        RTFREE(ron->ro_rt);
        ron->ro_rt = NULL;
    }    
    return (NULL);
}

static int
in6_selectif(dstsock, opts, ro, retifp)
	struct sockaddr_in6 *dstsock;
	struct ip6_pktopts *opts;
	struct route_in6 *ro;
	struct ifnet **retifp;
{
	struct rtentry *rt;
	int error;

	rt = in6_rtentry(opts, retifp, &error);
	if (rt == NULL) {
		return (error);
	}
	error = selectroute(dstsock, opts, ro, rt, retifp);
	if (error != 0) {
		return (error);
	}

	/*
	 * do not use a rejected or black hole route.
	 * XXX: this check should be done in the L2 output routine.
	 * However, if we skipped this check here, we'd see the following
	 * scenario:
	 * - install a rejected route for a scoped address prefix
	 *   (like fe80::/10)
	 * - send a packet to a destination that matches the scoped prefix,
	 *   with ambiguity about the scope zone.
	 * - pick the outgoing interface from the route, and disambiguate the
	 *   scope zone with the interface.
	 * - ip6_output() would try to get another route with the "new"
	 *   destination, which may be valid.
	 * - we'd see no error on output.
	 * Although this may not be very harmful, it should still be confusing.
	 * We thus reject the case here.
	 */
	if (rt && (rt->rt_flags & (RTF_REJECT | RTF_BLACKHOLE))) {
		return (rt->rt_flags & RTF_HOST ? EHOSTUNREACH : ENETUNREACH);
	}

	/*
	 * Adjust the "outgoing" interface.  If we're going to loop the packet
	 * back to ourselves, the ifp would be the loopback interface.
	 * However, we'd rather know the interface associated to the
	 * destination address (which should probably be one of our own
	 * addresses.)
	 */
	if (rt && rt->rt_ifa && rt->rt_ifa->ifa_ifp) {
		*retifp = rt->rt_ifa->ifa_ifp;
	}
	return (0);
}

/*
 * Default hop limit selection. The precedence is as follows:
 * 1. Hoplimit value specified via ioctl.
 * 2. (If the outgoing interface is detected) the current
 *     hop limit of the interface specified by router advertisement.
 * 3. The system default hoplimit.
*/
int
in6_selecthlim(in6p, ifp)
	struct in6pcb *in6p;
	struct ifnet *ifp;
{
	if (in6p && in6p->in6p_af != AF_INET6)
		return (-1);

	if (in6p && in6p->in6p_hops >= 0)
		return (in6p->in6p_hops);
	else if (ifp)
		return (ND_IFINFO(ifp)->chlim);
	else
		return (ip6_defhlim);
}

/*
 * Find an empty port and set it to the specified PCB.
 */
int
in6_pcbsetport(laddr, in6p, p)
	struct in6_addr *laddr;
	struct in6pcb *in6p;
	struct proc *p;
{
	struct socket *so = in6p->in6p_socket;
	struct inpcbtable *table = in6p->in6p_table;
	int cnt;
	u_int16_t min, max;
	u_int16_t lport, *lastport;
	int wild = 0;
	void *t;

	if (in6p->in6p_af != AF_INET6)
		return (EINVAL);

	/* XXX: this is redundant when called from in6_pcbbind */
	if ((so->so_options & (SO_REUSEADDR|SO_REUSEPORT)) == 0 &&
	   ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
	    (so->so_options & SO_ACCEPTCONN) == 0))
		wild = 1;

	if (in6p->in6p_flags & IN6P_LOWPORT) {
#ifndef IPNOPRIVPORTS
		if (p == 0 || (suser(p->p_ucred, &p->p_acflag) != 0))
			return (EACCES);
#endif
		min = ip6_lowportmin;
		max = ip6_lowportmax;
		lastport = &table->inpt_lastlow;
	} else {
		min = ip6_anonportmin;
		max = ip6_anonportmax;
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
#ifdef INET
		if (IN6_IS_ADDR_V4MAPPED(laddr)) {
			t = in_pcblookup_port(table,
			    *(struct in_addr *)&in6p->in6p_laddr.s6_addr32[3],
			    lport, wild);
		} else
#endif
		{
			t = in6_pcblookup_port(table, laddr, lport, wild);
		}
		if (t == 0)
			goto found;
	}

	return (EAGAIN);

found:
	in6p->in6p_flags |= IN6P_ANONPORT;
	*lastport = lport;
	in6p->in6p_lport = htons(lport);
	in6_pcbstate(in6p, IN6P_BOUND);
	return (0);		/* success */
}
