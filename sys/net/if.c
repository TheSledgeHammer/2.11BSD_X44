/*	$NetBSD: if.c,v 1.139.2.1.4.1 2006/11/19 17:30:11 bouyer Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by William Studnemund and Jason R. Thorpe.
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
 * Copyright (c) 1980, 1986, 1993
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
 *	@(#)if.c	8.5 (Berkeley) 1/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if.c,v 1.139.2.1.4.1 2006/11/19 17:30:11 bouyer Exp $");

#include "opt_inet.h"

#include "opt_pfil_hooks.h"

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_ioctl.h>
#include <net/if_types.h>
#include <net/radix.h>
#include <net/route.h>
#include <net/netisr.h>

#include <net/pf/ifg_group.h>

#ifdef INET6
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#endif

#include "carp.h"
#if NCARP > 0
#include <netinet/ip_carp.h>
#endif

int	ifqmaxlen = IFQ_MAXLEN;
struct	callout if_slowtimo_ch;

int netisr;			/* scheduling bits for network */

int if_rt_walktree(struct radix_node *, void *);

struct if_clone *if_clone_lookup(const char *, int *);
int if_clone_list(struct if_clonereq *);

LIST_HEAD(, if_clone) if_cloners = LIST_HEAD_INITIALIZER(if_cloners);
int if_cloners_count;

#if defined(INET) || defined(INET6) || defined(NS)
static void if_detach_queues(struct ifnet *, struct ifqueue *);
#endif

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */
void
ifinit(void)
{
	callout_init(&if_slowtimo_ch);
	if_slowtimo(NULL);
}

/*
 * Null routines used while an interface is going away.  These routines
 * just return an error.
 */

int
if_nulloutput(ifp, m, so, rt)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *so;
	struct rtentry *rt;
{

	return (ENXIO);
}

void
if_nullinput(ifp, m)
	struct ifnet *ifp;
	struct mbuf *m;
{

	/* Nothing. */
}

void
if_nullstart(ifp)
	struct ifnet *ifp;
{

	/* Nothing. */
}

int
if_nullioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{

	return (ENXIO);
}

int
if_nullinit(ifp)
	struct ifnet *ifp;
{

	return (ENXIO);
}

void
if_nullstop(ifp, disable)
	struct ifnet *ifp;
	int disable;
{

	/* Nothing. */
}

void
if_nullwatchdog(ifp)
	struct ifnet *ifp;
{

	/* Nothing. */
}

void
if_nulldrain(ifp)
	struct ifnet *ifp;
{

	/* Nothing. */
}

static u_int if_index = 1;
struct ifnet_head ifnet;
size_t if_indexlim = 0;
struct ifaddr **ifnet_addrs = NULL;
struct ifnet **ifindex2ifnet = NULL;

void
if_set_sadl(struct ifnet *ifp, void *addr, u_char lsap, u_char addrlen)
{
	struct sockaddr_dl *sdl;

    ifp->if_addrlen = addrlen;
    if_alloc_sadl(ifp);
    sdl = ifp->if_sadl;
    (void)sockaddr_dl_setaddrif(ifp, addr, lsap, addrlen, sdl, sdl->sdl_family);
}

/*
 * Allocate the link level name for the specified interface.  This
 * is an attachment helper.  It must be called after ifp->if_addrlen
 * is initialized, which may not be the case when if_attach() is
 * called.
 */
void
if_alloc_sadl(struct ifnet *ifp)
{
	unsigned socksize, ifasize;
	int namelen, masklen;
	struct sockaddr_dl *sdl;
	struct ifaddr *ifa;

#define ROUNDUP(a) (1 + (((a) - 1) | (sizeof(long) - 1)))

	/*
	 * If the interface already has a link name, release it
	 * now.  This is useful for interfaces that can change
	 * link types, and thus switch link names often.
	 */
	if (ifp->if_sadl != NULL) {
		if_free_sadl(ifp);
	}

	namelen = strlen(ifp->if_xname);
	masklen = offsetof(struct sockaddr_dl, sdl_data[0]) + namelen;
	socksize = masklen + ifp->if_addrlen;
	if (socksize < sizeof(*sdl)) {
		socksize = sizeof(*sdl);
	}
	socksize = ROUNDUP(socksize);
	ifasize = sizeof(*ifa) + 2 * socksize;
	ifa = (struct ifaddr *)malloc(ifasize, M_IFADDR, M_WAITOK);
	memset((caddr_t)ifa, 0, ifasize);
	sdl = (struct sockaddr_dl *)(ifa + 1);
	sdl->sdl_len = socksize;
	sdl->sdl_family = AF_LINK;
	bcopy(ifp->if_xname, sdl->sdl_data, namelen);
	sdl->sdl_nlen = namelen;
	sdl->sdl_alen = ifp->if_addrlen;
	sdl->sdl_index = ifp->if_index;
	sdl->sdl_type = ifp->if_type;
	ifnet_addrs[ifp->if_index] = ifa;
	IFAREF(ifa);
	ifa->ifa_ifp = ifp;
	ifa->ifa_rtrequest = link_rtrequest;
	TAILQ_INSERT_HEAD(&ifp->if_addrlist, ifa, ifa_list);
	IFAREF(ifa);
	ifa->ifa_addr = (struct sockaddr *)sdl;
	ifp->if_sadl = sdl;
	sdl = (struct sockaddr_dl *)(socksize + (caddr_t)sdl);
	ifa->ifa_netmask = (struct sockaddr *)sdl;
	sdl->sdl_len = masklen;
	while (namelen != 0) {
		sdl->sdl_data[--namelen] = 0xff;
	}
}

/*
 * Free the link level name for the specified interface.  This is
 * a detach helper.  This is called from if_detach() or from
 * link layer type specific detach functions.
 */
void
if_free_sadl(struct ifnet *ifp)
{
	struct ifaddr *ifa;
	int s;

	ifa = ifnet_addrs[ifp->if_index];
	if (ifa == NULL) {
		KASSERT(ifp->if_sadl == NULL);
		return;
	}

	KASSERT(ifp->if_sadl != NULL);

	s = splnet();
	rtinit(ifa, RTM_DELETE, 0);
	TAILQ_REMOVE(&ifp->if_addrlist, ifa, ifa_list);
	IFAFREE(ifa);

	ifp->if_sadl = NULL;

	ifnet_addrs[ifp->if_index] = NULL;
	IFAFREE(ifa);
	splx(s);
}

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
void
if_attach(ifp)
	struct ifnet *ifp;
{
	int indexlim = 0;

	if (if_indexlim == 0) {
		TAILQ_INIT(&ifnet);
		if_indexlim = 8;
	}
	TAILQ_INIT(&ifp->if_addrlist);
	TAILQ_INSERT_TAIL(&ifnet, ifp, if_list);
	ifp->if_index = if_index;
	if (ifindex2ifnet == 0)
		if_index++;
	else
		while (ifp->if_index < if_indexlim &&
		    ifindex2ifnet[ifp->if_index] != NULL) {
			++if_index;
			if (if_index == 0)
				if_index = 1;
			/*
			 * If we hit USHRT_MAX, we skip back to 0 since
			 * there are a number of places where the value
			 * of if_index or if_index itself is compared
			 * to or stored in an unsigned short.  By
			 * jumping back, we won't botch those assignments
			 * or comparisons.
			 */
			else if (if_index == USHRT_MAX) {
				/*
				 * However, if we have to jump back to
				 * zero *twice* without finding an empty
				 * slot in ifindex2ifnet[], then there
				 * there are too many (>65535) interfaces.
				 */
				if (indexlim++)
					panic("too many interfaces");
				else
					if_index = 1;
			}
			ifp->if_index = if_index;
		}

	/*
	 * We have some arrays that should be indexed by if_index.
	 * since if_index will grow dynamically, they should grow too.
	 *	struct ifadd **ifnet_addrs
	 *	struct ifnet **ifindex2ifnet
	 */
	if (ifnet_addrs == 0 || ifindex2ifnet == 0 ||
	    ifp->if_index >= if_indexlim) {
		size_t m, n, oldlim;
		caddr_t q;
		
		oldlim = if_indexlim;
		while (ifp->if_index >= if_indexlim)
			if_indexlim <<= 1;

		/* grow ifnet_addrs */
		m = oldlim * sizeof(struct ifaddr *);
		n = if_indexlim * sizeof(struct ifaddr *);
		q = (caddr_t)malloc(n, M_IFADDR, M_WAITOK);
		memset(q, 0, n);
		if (ifnet_addrs) {
			bcopy((caddr_t)ifnet_addrs, q, m);
			free((caddr_t)ifnet_addrs, M_IFADDR);
		}
		ifnet_addrs = (struct ifaddr **)q;

		/* grow ifindex2ifnet */
		m = oldlim * sizeof(struct ifnet *);
		n = if_indexlim * sizeof(struct ifnet *);
		q = (caddr_t)malloc(n, M_IFADDR, M_WAITOK);
		memset(q, 0, n);
		if (ifindex2ifnet) {
			bcopy((caddr_t)ifindex2ifnet, q, m);
			free((caddr_t)ifindex2ifnet, M_IFADDR);
		}
		ifindex2ifnet = (struct ifnet **)q;
	}

	ifindex2ifnet[ifp->if_index] = ifp;

	/*
	 * Link level name is allocated later by a separate call to
	 * if_alloc_sadl().
	 */

	if (ifp->if_snd.ifq_maxlen == 0)
		ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_broadcastaddr = 0; /* reliably crash if used uninitialized */

	ifp->if_link_state = LINK_STATE_UNKNOWN;

	ifp->if_capenable = 0;
	ifp->if_csum_flags_tx = 0;
	ifp->if_csum_flags_rx = 0;

#ifdef ALTQ
	ifp->if_snd.altq_type = 0;
	ifp->if_snd.altq_disc = NULL;
	ifp->if_snd.altq_flags &= ALTQF_CANTCHANGE;
	ifp->if_snd.altq_tbr  = NULL;
	ifp->if_snd.altq_ifp  = ifp;
#endif

#ifdef PFIL_HOOKS
	ifp->if_pfil.ph_type = PFIL_TYPE_IFNET;
	ifp->if_pfil.ph_ifnet = ifp;
	if (pfil_head_register(&ifp->if_pfil) != 0)
		printf("%s: WARNING: unable to register pfil hook\n",
		    ifp->if_xname);
#endif

	if (domains) {
		if_attachdomain1(ifp);
	}

	/* Announce the interface. */
	rt_ifannouncemsg(ifp, IFAN_ARRIVAL);
}

void
if_attachdomain(void)
{
	struct ifnet *ifp;
	int s;

	s = splnet();
	for (ifp = TAILQ_FIRST(&ifnet); ifp; ifp = TAILQ_NEXT(ifp, if_list)) {
		if_attachdomain1(ifp);
	}
	splx(s);
}

void
if_attachdomain1(ifp)
	struct ifnet *ifp;
{
	struct domain *dp;
	int s;

	s = splnet();

	/* address family dependent data region */
	memset(ifp->if_afdata, 0, sizeof(ifp->if_afdata));
	for (dp = domains; dp; dp = dp->dom_next) {
		if (dp->dom_ifattach)
			ifp->if_afdata[dp->dom_family] =
			    (*dp->dom_ifattach)(ifp);
	}

	splx(s);
}

/*
 * Deactivate an interface.  This points all of the procedure
 * handles at error stubs.  May be called from interrupt context.
 */
void
if_deactivate(ifp)
	struct ifnet *ifp;
{
	int s;

	s = splnet();

	ifp->if_output	 = if_nulloutput;
	ifp->if_input	 = if_nullinput;
	ifp->if_start	 = if_nullstart;
	ifp->if_ioctl	 = if_nullioctl;
	ifp->if_init	 = if_nullinit;
	ifp->if_stop	 = if_nullstop;
	ifp->if_watchdog = if_nullwatchdog;
	ifp->if_drain	 = if_nulldrain;

	/* No more packets may be enqueued. */
	ifp->if_snd.ifq_maxlen = 0;

	splx(s);
}

/*
 * Detach an interface from the list of "active" interfaces,
 * freeing any resources as we go along.
 *
 * NOTE: This routine must be called with a valid thread context,
 * as it may block.
 */
void
if_detach(ifp)
	struct ifnet *ifp;
{
	struct socket so;
	struct ifaddr *ifa, **ifap;
#ifdef IFAREF_DEBUG
	struct ifaddr *last_ifa = NULL;
#endif
	struct domain *dp;
	struct protosw *pr;
	struct radix_node_head *rnh;
	int s, i, family, purged;

	/*
	 * XXX It's kind of lame that we have to have the
	 * XXX socket structure...
	 */
	memset(&so, 0, sizeof(so));

	s = splnet();

	/*
	 * Do an if_down() to give protocols a chance to do something.
	 */
	if_down(ifp);

#ifdef ALTQ
	if (ALTQ_IS_ENABLED(&ifp->if_snd))
		altq_disable(&ifp->if_snd);
	if (ALTQ_IS_ATTACHED(&ifp->if_snd))
		altq_detach(&ifp->if_snd);
#endif
#if NCARP > 0
	if (ifp->if_carp != NULL && ifp->if_type != IFT_CARP) {
		carp_ifdetach(ifp);
	}
#endif
#ifdef PFIL_HOOKS
	(void) pfil_head_unregister(&ifp->if_pfil);
#endif

	/*
	 * Rip all the addresses off the interface.  This should make
	 * all of the routes go away.
	 */
	ifap = &TAILQ_FIRST(&ifp->if_addrlist); /* XXX abstraction violation */
	while ((ifa = *ifap)) {
		family = ifa->ifa_addr->sa_family;
#ifdef IFAREF_DEBUG
		printf("if_detach: ifaddr %p, family %d, refcnt %d\n",
		    ifa, family, ifa->ifa_refcnt);
		if (last_ifa != NULL && ifa == last_ifa)
			panic("if_detach: loop detected");
		last_ifa = ifa;
#endif
		if (family == AF_LINK) {
			ifap = &TAILQ_NEXT(ifa, ifa_list);
			continue;
		}
		dp = pffinddomain(family);
#ifdef DIAGNOSTIC
		if (dp == NULL)
			panic("if_detach: no domain for AF %d",
			    family);
#endif
		purged = 0;
		for (pr = dp->dom_protosw;
		     pr < dp->dom_protoswNPROTOSW; pr++) {
			so.so_proto = pr;
			if (pr->pr_usrreq != NULL) {
				(void) (*pr->pr_usrreq)(&so, PRU_PURGEIF, NULL, NULL, (struct mbuf *) ifp, curproc);
				purged = 1;
			}
		}
		if (purged == 0) {
			/*
			 * XXX What's really the best thing to do
			 * XXX here?  --thorpej@NetBSD.org
			 */
			printf("if_detach: WARNING: AF %d not purged\n",
			    family);
			TAILQ_REMOVE(&ifp->if_addrlist, ifa, ifa_list);
		}
	}

	if_free_sadl(ifp);

	/* Walk the routing table looking for straglers. */
	for (i = 0; i <= AF_MAX; i++) {
		if ((rnh = rt_tables[i]) != NULL)
			(void) (*rnh->rnh_walktree)(rnh, if_rt_walktree, ifp);
	}

	for (dp = domains; dp; dp = dp->dom_next) {
		if (dp->dom_ifdetach && ifp->if_afdata[dp->dom_family])
			(*dp->dom_ifdetach)(ifp,
			    ifp->if_afdata[dp->dom_family]);
	}

	/* Announce that the interface is gone. */
	rt_ifannouncemsg(ifp, IFAN_DEPARTURE);

	ifindex2ifnet[ifp->if_index] = NULL;

	TAILQ_REMOVE(&ifnet, ifp, if_list);

	/*
	 * remove packets came from ifp, from software interrupt queues.
	 * net/netisr_dispatch.h is not usable, as some of them use
	 * strange queue names.
	 */
#define IF_DETACH_QUEUES(x) 	\
do { 							\
	extern struct ifqueue x; 	\
	if_detach_queues(ifp, & x); \
} while (/*CONSTCOND*/ 0)
#ifdef INET
#if NARP > 0
	IF_DETACH_QUEUES(arpintrq);
#endif
	IF_DETACH_QUEUES(ipintrq);
#endif
#ifdef INET6
	IF_DETACH_QUEUES(ip6intrq);
#endif
#ifdef NS
	IF_DETACH_QUEUES(nsintrq);
#endif
#undef IF_DETACH_QUEUES

	splx(s);
}

#if defined(INET) || defined(INET6) || defined(NS)
static void
if_detach_queues(ifp, q)
	struct ifnet *ifp;
	struct ifqueue *q;
{
	struct mbuf *m, *prev, *next;

	prev = NULL;
	for (m = q->ifq_head; m; m = next) {
		next = m->m_nextpkt;
#ifdef DIAGNOSTIC
		if ((m->m_flags & M_PKTHDR) == 0) {
			prev = m;
			continue;
		}
#endif
		if (m->m_pkthdr.rcvif != ifp) {
			prev = m;
			continue;
		}

		if (prev)
			prev->m_nextpkt = m->m_nextpkt;
		else
			q->ifq_head = m->m_nextpkt;
		if (q->ifq_tail == m)
			q->ifq_tail = prev;
		q->ifq_len--;

		m->m_nextpkt = NULL;
		m_freem(m);
		IF_DROP(q);
	}
}
#endif /* defined(INET) || ... */

/*
 * Callback for a radix tree walk to delete all references to an
 * ifnet.
 */
int
if_rt_walktree(rn, v)
	struct radix_node *rn;
	void *v;
{
	struct ifnet *ifp = (struct ifnet *)v;
	struct rtentry *rt = (struct rtentry *)rn;
	int error;

	if (rt->rt_ifp == ifp) {
		/* Delete the entry. */
		error = rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway,
		    rt_mask(rt), rt->rt_flags, NULL);
		if (error)
			printf("%s: warning: unable to delete rtentry @ %p, "
			    "error = %d\n", ifp->if_xname, rt, error);
	}
	return (0);
}

/*
 * Create a clone network interface.
 */
int
if_clone_create(name)
	const char *name;
{
	struct if_clone *ifc;
	int unit;

	ifc = if_clone_lookup(name, &unit);
	if (ifc == NULL)
		return (EINVAL);

	if (ifunit(name) != NULL)
		return (EEXIST);

	return ((*ifc->ifc_create)(ifc, unit));
}

/*
 * Destroy a clone network interface.
 */
int
if_clone_destroy(name)
	const char *name;
{
	struct if_clone *ifc;
	struct ifnet *ifp;

	ifc = if_clone_lookup(name, NULL);
	if (ifc == NULL)
		return (EINVAL);

	ifp = ifunit(name);
	if (ifp == NULL)
		return (ENXIO);

	if (ifc->ifc_destroy == NULL)
		return (EOPNOTSUPP);

	(*ifc->ifc_destroy)(ifp);
	return (0);
}

/*
 * Look up a network interface cloner.
 */
struct if_clone *
if_clone_lookup(name, unitp)
	const char *name;
	int *unitp;
{
	struct if_clone *ifc;
	const char *cp;
	int unit;

	/* separate interface name from unit */
	for (cp = name;
	    cp - name < IFNAMSIZ && *cp && (*cp < '0' || *cp > '9');
	    cp++)
		continue;

	if (cp == name || cp - name == IFNAMSIZ || !*cp)
		return (NULL);	/* No name or unit number */

	LIST_FOREACH(ifc, &if_cloners, ifc_list) {
		if (strlen(ifc->ifc_name) == cp - name &&
		    !strncmp(name, ifc->ifc_name, cp - name))
			break;
	}

	if (ifc == NULL)
		return (NULL);

	unit = 0;
	while (cp - name < IFNAMSIZ && *cp) {
		if (*cp < '0' || *cp > '9' || unit > INT_MAX / 10) {
			/* Bogus unit number. */
			return (NULL);
		}
		unit = (unit * 10) + (*cp++ - '0');
	}

	if (unitp != NULL)
		*unitp = unit;
	return (ifc);
}

/*
 * Register a network interface cloner.
 */
void
if_clone_attach(ifc)
	struct if_clone *ifc;
{

	LIST_INSERT_HEAD(&if_cloners, ifc, ifc_list);
	if_cloners_count++;
}

/*
 * Unregister a network interface cloner.
 */
void
if_clone_detach(ifc)
	struct if_clone *ifc;
{

	LIST_REMOVE(ifc, ifc_list);
	if_cloners_count--;
}

/*
 * Provide list of interface cloners to userspace.
 */
int
if_clone_list(ifcr)
	struct if_clonereq *ifcr;
{
	char outbuf[IFNAMSIZ], *dst;
	struct if_clone *ifc;
	int count, error = 0;

	ifcr->ifcr_total = if_cloners_count;
	if ((dst = ifcr->ifcr_buffer) == NULL) {
		/* Just asking how many there are. */
		return (0);
	}

	if (ifcr->ifcr_count < 0)
		return (EINVAL);

	count = (if_cloners_count < ifcr->ifcr_count) ?
	    if_cloners_count : ifcr->ifcr_count;

	for (ifc = LIST_FIRST(&if_cloners); ifc != NULL && count != 0;
	     ifc = LIST_NEXT(ifc, ifc_list), count--, dst += IFNAMSIZ) {
		(void)strncpy(outbuf, ifc->ifc_name, sizeof(outbuf));
		if (outbuf[sizeof(outbuf) - 1] != '\0')
			return ENAMETOOLONG;
		error = copyout(outbuf, dst, sizeof(outbuf));
		if (error)
			break;
	}

	return (error);
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithaddr(addr)
	struct sockaddr *addr;
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

#define	equal(a1, a2) \
  (bcmp((caddr_t)(a1), (caddr_t)(a2), ((struct sockaddr *)(a1))->sa_len) == 0)

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
		for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
		     ifa = TAILQ_NEXT(ifa, ifa_list)) {
			if (ifa->ifa_addr->sa_family != addr->sa_family)
				continue;
			if (equal(addr, ifa->ifa_addr))
				return (ifa);
			if ((ifp->if_flags & IFF_BROADCAST) &&
			    ifa->ifa_broadaddr &&
			    /* IP6 doesn't have broadcast */
			    ifa->ifa_broadaddr->sa_len != 0 &&
			    equal(ifa->ifa_broadaddr, addr))
				return (ifa);
		}
	}
	return (NULL);
}

/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr)
	struct sockaddr *addr;
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
		if (ifp->if_flags & IFF_POINTOPOINT) {
			for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
			     ifa = TAILQ_NEXT(ifa, ifa_list)) {
				if (ifa->ifa_addr->sa_family !=
				      addr->sa_family ||
				    ifa->ifa_dstaddr == NULL)
					continue;
				if (equal(addr, ifa->ifa_dstaddr))
					return (ifa);
			}
		}
	}
	return (NULL);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is most specific found.
 */
struct ifaddr *
ifa_ifwithnet(addr)
	struct sockaddr *addr;
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct sockaddr_dl *sdl;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;
	char *addr_data = addr->sa_data, *cplim;

	if (af == AF_LINK) {
		sdl = (struct sockaddr_dl *)addr;
		if (sdl->sdl_index && sdl->sdl_index < if_indexlim &&
		    ifindex2ifnet[sdl->sdl_index] &&
		    ifindex2ifnet[sdl->sdl_index]->if_output != if_nulloutput)
			return (ifnet_addrs[sdl->sdl_index]);
	}
	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
		for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
		     ifa = TAILQ_NEXT(ifa, ifa_list)) {
			char *cp, *cp2, *cp3;

			if (ifa->ifa_addr->sa_family != af ||
			    ifa->ifa_netmask == 0)
 next:				continue;
			cp = addr_data;
			cp2 = ifa->ifa_addr->sa_data;
			cp3 = ifa->ifa_netmask->sa_data;
			cplim = (char *)ifa->ifa_netmask +
			    ifa->ifa_netmask->sa_len;
			while (cp3 < cplim) {
				if ((*cp++ ^ *cp2++) & *cp3++) {
					/* want to continue for() loop */
					goto next;
				}
			}
			if (ifa_maybe == 0 ||
			    rn_refines((caddr_t)ifa->ifa_netmask,
			    (caddr_t)ifa_maybe->ifa_netmask))
				ifa_maybe = ifa;
		}
	}
	return (ifa_maybe);
}

/*
 * Find the interface of the addresss.
 */
struct ifaddr *
ifa_ifwithladdr(addr)
	struct sockaddr *addr;
{
	struct ifaddr *ia;

	if ((ia = ifa_ifwithaddr(addr)) || (ia = ifa_ifwithdstaddr(addr)) ||
	    (ia = ifa_ifwithnet(addr)))
		return (ia);
	return (NULL);
}

/*
 * Find an interface using a specific address family
 */
struct ifaddr *
ifa_ifwithaf(af)
	int af;
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
		for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
		     ifa = TAILQ_NEXT(ifa, ifa_list)) {
			if (ifa->ifa_addr->sa_family == af)
				return (ifa);
		}
	}
	return (NULL);
}

/*
 * Find an interface address specific to an interface best matching
 * a given address.
 */
struct ifaddr *
ifaof_ifpforaddr(addr, ifp)
	struct sockaddr *addr;
	struct ifnet *ifp;
{
	struct ifaddr *ifa;
	char *cp, *cp2, *cp3;
	char *cplim;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;

	if (ifp->if_output == if_nulloutput)
		return (NULL);

	if (af >= AF_MAX)
		return (NULL);

	for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
	     ifa = TAILQ_NEXT(ifa, ifa_list)) {
		if (ifa->ifa_addr->sa_family != af)
			continue;
		ifa_maybe = ifa;
		if (ifa->ifa_netmask == 0) {
			if (equal(addr, ifa->ifa_addr) ||
			    (ifa->ifa_dstaddr &&
			     equal(addr, ifa->ifa_dstaddr)))
				return (ifa);
			continue;
		}
		cp = addr->sa_data;
		cp2 = ifa->ifa_addr->sa_data;
		cp3 = ifa->ifa_netmask->sa_data;
		cplim = ifa->ifa_netmask->sa_len + (char *)ifa->ifa_netmask;
		for (; cp3 < cplim; cp3++) {
			if ((*cp++ ^ *cp2++) & *cp3)
				break;
		}
		if (cp3 == cplim)
			return (ifa);
	}
	return (ifa_maybe);
}

/*
 * Default action when installing a route with a Link Level gateway.
 * Lookup an appropriate real ifa to point to.
 * This should be moved to /sys/net/link.c eventually.
 */
void
link_rtrequest(cmd, rt, info)
	int cmd;
	struct rtentry *rt;
	struct rt_addrinfo *info;
{
	struct ifaddr *ifa;
	struct sockaddr *dst;
	struct ifnet *ifp;

	if (cmd != RTM_ADD || ((ifa = rt->rt_ifa) == 0) ||
	    ((ifp = ifa->ifa_ifp) == 0) || ((dst = rt_key(rt)) == 0))
		return;
	if ((ifa = ifaof_ifpforaddr(dst, ifp)) != NULL) {
		IFAFREE(rt->rt_ifa);
		rt->rt_ifa = ifa;
		IFAREF(ifa);
		if (ifa->ifa_rtrequest && ifa->ifa_rtrequest != link_rtrequest)
			ifa->ifa_rtrequest(cmd, rt, info);
	}
}

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splsoftnet or equivalent.
 */
void
if_down(ifp)
	struct ifnet *ifp;
{
	struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	microtime(&ifp->if_lastchange);
	for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
	     ifa = TAILQ_NEXT(ifa, ifa_list)) {
		pfctlinput(PRC_IFDOWN, ifa->ifa_addr);
	}
	IFQ_PURGE(&ifp->if_snd);
#if NCARP > 0
	if (ifp->if_carp) {
		carp_carpdev_state(ifp);
	}
#endif
	rt_ifmsg(ifp);
}

/*
 * Mark an interface up and notify protocols of
 * the transition.
 * NOTE: must be called at splsoftnet or equivalent.
 */
void
if_up(ifp)
	struct ifnet *ifp;
{
#ifdef notyet
	struct ifaddr *ifa;
#endif

	ifp->if_flags |= IFF_UP;
	microtime(&ifp->if_lastchange);
#ifdef notyet
	/* this has no effect on IP, and will kill all ISO connections XXX */
	for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
	     ifa = TAILQ_NEXT(ifa, ifa_list)) {
		pfctlinput(PRC_IFUP, ifa->ifa_addr);
	}
#endif
#if NCARP > 0
	if (ifp->if_carp) {
		carp_carpdev_state(ifp);
	}
#endif
	rt_ifmsg(ifp);
#ifdef INET6
	in6_if_up(ifp);
#endif
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
void
if_slowtimo(arg)
	void *arg;
{
	struct ifnet *ifp;
	int s = splnet();

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp);
	}
	splx(s);
	callout_reset(&if_slowtimo_ch, hz / IFNET_SLOWHZ,
	    if_slowtimo, NULL);
}

/*
 * Set/clear promiscuous mode on interface ifp based on the truth value
 * of pswitch.  The calls are reference counted so that only the first
 * "on" request actually has an effect, as does the final "off" request.
 * Results are undefined if the "off" and "on" requests are not matched.
 */
int
ifpromisc(ifp, pswitch)
	struct ifnet *ifp;
	int pswitch;
{
	int pcount, ret;
	short flags;
	struct ifreq ifr;

	pcount = ifp->if_pcount;
	flags = ifp->if_flags;
	if (pswitch) {
		/*
		 * Allow the device to be "placed" into promiscuous
		 * mode even if it is not configured up.  It will
		 * consult IFF_PROMISC when it is is brought up.
		 */
		if (ifp->if_pcount++ != 0)
			return (0);
		ifp->if_flags |= IFF_PROMISC;
		if ((ifp->if_flags & IFF_UP) == 0)
			return (0);
	} else {
		if (--ifp->if_pcount > 0)
			return (0);
		ifp->if_flags &= ~IFF_PROMISC;
		/*
		 * If the device is not configured up, we should not need to
		 * turn off promiscuous mode (device should have turned it
		 * off when interface went down; and will look at IFF_PROMISC
		 * again next time interface comes up).
		 */
		if ((ifp->if_flags & IFF_UP) == 0)
			return (0);
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = ifp->if_flags;
	ret = (*ifp->if_ioctl)(ifp, SIOCSIFFLAGS, (caddr_t) &ifr);
	/* Restore interface state if not successful. */
	if (ret != 0) {
		ifp->if_pcount = pcount;
		ifp->if_flags = flags;
	}
	return (ret);
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(name)
	const char *name;
{
	struct ifnet *ifp;
	const char *cp = name;
	u_int unit = 0;
	u_int i;

	/*
	 * If the entire name is a number, treat it as an ifindex.
	 */
	for (i = 0; i < IFNAMSIZ && *cp >= '0' && *cp <= '9'; i++, cp++) {
		unit = unit * 10 + (*cp - '0');
	}

	/*
	 * If the number took all of the name, then it's a valid ifindex.
	 */
	if (i == IFNAMSIZ || (cp != name && *cp == '\0')) {
		if (unit >= if_indexlim)
			return (NULL);
		ifp = ifindex2ifnet[unit];
		if (ifp == NULL || ifp->if_output == if_nulloutput)
			return (NULL);
		return (ifp);
	}

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
	 	if (strcmp(ifp->if_xname, name) == 0)
			return (ifp);
	}
	return (NULL);
}

/*
 * Interface ioctls.
 */
int
ifioctl(so, cmd, data, p)
	struct socket *so;
	u_long cmd;
	caddr_t data;
	struct proc *p;
{
	struct ifnet *ifp;
	struct ifreq *ifr;
	struct ifcapreq *ifcr;
	struct ifdatareq *ifdr;
	int s, error = 0;
	short oif_flags;

	ifr = (struct ifreq *)data;
	ifcr = (struct ifcapreq *)data;
	ifdr = (struct ifdatareq *)data;

	switch (cmd) {
	case SIOCIFCREATE:
		if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0) {
			return (error);
		}
		error = if_clone_create(ifr->ifr_name);
		return (error);

	case SIOCIFDESTROY:
		if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0) {
			return (error);
		}
		error = if_clone_destroy(ifr->ifr_name);
		return (error);

	case SIOCIFGCLONERS:
		return (if_clone_list((struct if_clonereq *)data));

	case SIOCGIFGMEMB:
	case SIOCGIFGATTR:
	case SIOCSIFGATTR:
	case SIOCGIFGLIST:
	case SIOCAIFGROUP:
	case SIOCGIFGROUP:
	case SIOCDIFGROUP:
		error = ifgioctl_get(cmd, data);
		return (error);
	}

	ifp = ifunit(ifr->ifr_name);
	if (ifp == 0){
		return (ENXIO);
	}
	oif_flags = ifp->if_flags;

	switch (cmd) {
	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCGIFMTU:
		ifr->ifr_mtu = ifp->if_mtu;
		break;

	case SIOCGIFDLT:
		ifr->ifr_dlt = ifp->if_dlt;
		break;

	case SIOCSIFFLAGS:
		if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		if ((ifp->if_flags & IFF_UP) && (ifr->ifr_flags & IFF_UP) == 0) {
			s = splnet();
			if_down(ifp);
			splx(s);
		}
		if ((ifr->ifr_flags & IFF_UP) && (ifp->if_flags & IFF_UP) == 0) {
			s = splnet();
			if_up(ifp);
			splx(s);
		}
		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		if (ifp->if_ioctl)
			(void) (*ifp->if_ioctl)(ifp, cmd, data);
		break;

	case SIOCGIFCAP:
		ifcr->ifcr_capabilities = ifp->if_capabilities;
		ifcr->ifcr_capenable = ifp->if_capenable;
		break;

	case SIOCSIFCAP:
		if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		if ((ifcr->ifcr_capenable & ~ifp->if_capabilities) != 0)
			return (EINVAL);
		if (ifp->if_ioctl == NULL)
			return (EOPNOTSUPP);

		/* Must prevent race with packet reception here. */
		s = splnet();
		if (ifcr->ifcr_capenable != ifp->if_capenable) {
			struct ifreq ifrq;

			ifrq.ifr_flags = ifp->if_flags;
			ifp->if_capenable = ifcr->ifcr_capenable;

			/* Pre-compute the checksum flags mask. */
			ifp->if_csum_flags_tx = 0;
			ifp->if_csum_flags_rx = 0;
			if (ifp->if_capenable & IFCAP_CSUM_IPv4) {
				ifp->if_csum_flags_tx |= M_CSUM_IPv4;
				ifp->if_csum_flags_rx |= M_CSUM_IPv4;
			}

			if (ifp->if_capenable & IFCAP_CSUM_TCPv4) {
				ifp->if_csum_flags_tx |= M_CSUM_TCPv4;
				ifp->if_csum_flags_rx |= M_CSUM_TCPv4;
			} else if (ifp->if_capenable & IFCAP_CSUM_TCPv4_Rx)
				ifp->if_csum_flags_rx |= M_CSUM_TCPv4;

			if (ifp->if_capenable & IFCAP_CSUM_UDPv4) {
				ifp->if_csum_flags_tx |= M_CSUM_UDPv4;
				ifp->if_csum_flags_rx |= M_CSUM_UDPv4;
			} else if (ifp->if_capenable & IFCAP_CSUM_UDPv4_Rx)
				ifp->if_csum_flags_rx |= M_CSUM_UDPv4;

			if (ifp->if_capenable & IFCAP_CSUM_TCPv6) {
				ifp->if_csum_flags_tx |= M_CSUM_TCPv6;
				ifp->if_csum_flags_rx |= M_CSUM_TCPv6;
			}

			if (ifp->if_capenable & IFCAP_CSUM_UDPv6) {
				ifp->if_csum_flags_tx |= M_CSUM_UDPv6;
				ifp->if_csum_flags_rx |= M_CSUM_UDPv6;
			}

			/*
			 * Only kick the interface if it's up.  If it's
			 * not up now, it will notice the cap enables
			 * when it is brought up later.
			 */
			if (ifp->if_flags & IFF_UP)
				(void) (*ifp->if_ioctl)(ifp, SIOCSIFFLAGS,
				    (caddr_t) &ifrq);
		}
		splx(s);
		break;

	case SIOCSIFMETRIC:
		if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		ifp->if_metric = ifr->ifr_metric;
		break;

	case SIOCGIFDATA:
		ifdr->ifdr_data = ifp->if_data;
		break;

	case SIOCZIFDATA:
		if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		ifdr->ifdr_data = ifp->if_data;
		/*
		 * Assumes that the volatile counters that can be
		 * zero'ed are at the end of if_data.
		 */
		memset(&ifp->if_data.ifi_ipackets, 0, sizeof(ifp->if_data) - offsetof(struct if_data, ifi_ipackets));
		break;

	case SIOCSIFMTU:
	{
		u_long oldmtu = ifp->if_mtu;

		error = suser1(p->p_ucred, &p->p_acflag);
		if (error)
			return (error);
		if (ifp->if_ioctl == NULL)
			return (EOPNOTSUPP);
		error = (*ifp->if_ioctl)(ifp, cmd, data);

		/*
		 * If the link MTU changed, do network layer specific procedure.
		 */
		if (ifp->if_mtu != oldmtu) {
#ifdef INET6
			nd6_setmtu(ifp);
#endif 
		}
		break;
	}
	case SIOCSIFPHYADDR:
	case SIOCDIFPHYADDR:
#ifdef INET6
	case SIOCSIFPHYADDR_IN6:
#endif
	case SIOCSLIFPHYADDR:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFMEDIA:
		if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		/* FALLTHROUGH */
	case SIOCGIFPSRCADDR:
	case SIOCGIFPDSTADDR:
	case SIOCGLIFPHYADDR:
	case SIOCGIFMEDIA:
		if (ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		error = (*ifp->if_ioctl)(ifp, cmd, data);
		break;

	case SIOCSDRVSPEC:  
	case SIOCS80211NWID:
	case SIOCS80211NWKEY:
	case SIOCS80211POWER:
	case SIOCS80211BSSID:
	case SIOCS80211CHANNEL:
		/* XXX:  need to pass proc pointer through to driver... */
		if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
	/* FALLTHROUGH */
	default:
		if (so->so_proto == 0) {
			return (EOPNOTSUPP);
		}
		error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
		    (struct mbuf *)cmd, (struct mbuf *)data,
		    (struct mbuf *)ifp, p));
		break;
	}

	if (((oif_flags ^ ifp->if_flags) & IFF_UP) != 0) {
#ifdef INET6
		if ((ifp->if_flags & IFF_UP) != 0) {
			s = splnet();
			in6_if_up(ifp);
			splx(s);
		}
#endif
	}

	return (error);
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/

int
ifconf(cmd, data)
	u_long cmd;
	caddr_t data;
{
	struct ifconf *ifc = (struct ifconf *)data;
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;
	const int sz = (int)sizeof(ifr);
	int sign;

	if ((ifrp = ifc->ifc_req) == NULL) {
		space = 0;
		sign = -1;
	} else {
		sign = 1;
	}
	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		(void)strncpy(ifr.ifr_name, ifp->if_xname, sizeof(ifr.ifr_name));
		if (ifr.ifr_name[sizeof(ifr.ifr_name) - 1] != '\0') {
			return ENAMETOOLONG;
		}
		if ((ifa = TAILQ_FIRST(&ifp->if_addrlist)) == 0) {
			memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			if (ifrp != NULL && space >= sz) {
				error = copyout(&ifr, ifrp, sz);
				if (error) {
					break;
				}
				ifrp++;
			}
			space -= sizeof(ifr) * sign;
			continue;
		}

		for (; ifa != 0; ifa = TAILQ_NEXT(ifa, ifa_list)) {
			struct sockaddr *sa = ifa->ifa_addr;

			if (sa->sa_len <= sizeof(*sa)) {
				ifr.ifr_addr = *sa;
				if (ifrp != NULL && space >= sz) {
					error = copyout(&ifr, ifrp, sz);
					ifrp++;
				}
			} else {
				space -= (sa->sa_len - sizeof(*sa)) * sign;
				if (ifrp != NULL && space >= sz) {
					error = copyout(&ifr, ifrp, sizeof(ifr.ifr_name));
					if (error == 0) {
						error = copyout(sa, &ifrp->ifr_addr, sa->sa_len);
					}
					ifrp = (struct ifreq*) (sa->sa_len + (caddr_t)& ifrp->ifr_addr);
				}
			}
			if (error) {
				break;
			}
			space -= sz * sign;
		}
	}
	if (ifrp != NULL) {
		ifc->ifc_len -= space;
	} else {
		ifc->ifc_len = space;
	}
	return (error);
}

#if defined(INET) || defined(INET6)
static int
sysctl_ifqctl(name, oldp, oldlenp, newp, newlen, ifq)
	int *name;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct ifqueue *ifq;
{
	int error;

	switch(name[0]) {
	case IFQCTL_IFQ:
		error = sysctl_rdstruct(oldp, oldlenp, newp, ifq, sizeof(struct ifqueue));
		break;
	case IFQCTL_LEN:
		error = sysctl_rdint(oldp, oldlenp, newp, ifq->ifq_len);
		break;
	case IFQCTL_MAXLEN:
		error = sysctl_int(oldp, oldlenp, newp, newlen, &ifq->ifq_maxlen);
		break;
#ifdef notyet
	case IFQCTL_PEAK:
		error = sysctl_rdint(oldp, oldlenp, newp, &ifq->ifq_peak);
		break;
#endif
	case IFQCTL_DROPS:
		error = sysctl_rdint(oldp, oldlenp, newp, ifq->ifq_drops);
		break;
	}
	return (error);
}

static int
sysctl_ifqctl_protocol(ipn, name, oldp, oldlenp, newp, newlen, ifq)
	int ipn, *name;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct ifqueue *ifq;
{
	int error;

	switch (ipn) {
	case IPPROTO_IP:
		error = sysctl_ifqctl(name, oldp, oldlenp, newp, newlen, ifq);
		break;
	case IPPROTO_IPV6:
		error = sysctl_ifqctl(name, oldp, oldlenp, newp, newlen, ifq);
		break;
	case IPPROTO_IDP:
		error = sysctl_ifqctl(name, oldp, oldlenp, newp, newlen, ifq);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

static int
sysctl_ifqctl_ip(ipn, qid, name, oldp, oldlenp, newp, newlen, ifq)
	int ipn, qid, *name;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct ifqueue *ifq;
{
	int error;

	switch(qid) {
	case IPCTL_IFQ:
		error = sysctl_ifqctl_protocol(ipn, name, oldp, oldlenp, newp, newlen, ifq);
		break;
	case IPV6CTL_IFQ:
		error = sysctl_ifqctl_protocol(ipn, name, oldp, oldlenp, newp, newlen, ifq);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

static int
sysctl_ifq(pf, ipn, qid, name, oldp, oldlenp, newp, newlen, ifq)
	int pf, ipn, qid, *name;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct ifqueue *ifq;
{
	int error;

	switch (pf) {
	case PF_INET:
		error = sysctl_ifqctl_ip(ipn, qid, name, oldp, oldlenp, newp, newlen, ifq);
		break;
	case PF_INET6:
		error = sysctl_ifqctl_ip(ipn, qid, name, oldp, oldlenp, newp, newlen, ifq);
		break;
	case PF_NS:
		error = sysctl_ifqctl_ip(ipn, qid, name, oldp, oldlenp, newp, newlen, ifq);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}
#endif /* INET || INET6 */

#ifdef INET
int
sysctl_ifq_ip4(name, oldp, oldlenp, newp, newlen)
	int *name;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	extern struct ifqueue ipintrq;

	return (sysctl_ifq(PF_INET, IPPROTO_IP, IPCTL_IFQ, name, oldp, oldlenp, newp, newlen, &ipintrq));
}
#endif /* INET */

#ifdef INET6
int
sysctl_ifq_ip6(name, oldp, oldlenp, newp, newlen)
	int *name;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	extern struct ifqueue ip6intrq;

	return (sysctl_ifq(PF_INET6, IPPROTO_IPV6, IPV6CTL_IFQ, name, oldp, oldlenp, newp, newlen, &ip6intrq));
}
#endif /* INET6 */

#ifdef NS
int
sysctl_ifq_ns(name, oldp, oldlenp, newp, newlen)
	int *name;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	extern struct ifqueue nsintrq;

	return (sysctl_ifq(PF_NS, IPPROTO_IDP, IPCTL_IFQ, name, oldp, oldlenp, newp, newlen, &nsintrq));
}
#endif /* NS */
