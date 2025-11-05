/*	$NetBSD: ns_pcb.c,v 1.20 2004/02/24 15:22:01 wiz Exp $	*/

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1993
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
 *	@(#)ns_pcb.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ns_pcb.c,v 1.20 2004/02/24 15:22:01 wiz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netns/ns.h>
#include <netns/ns_if.h>
//#include <netns/ns_pcb.h>
#include <netns/ns_var.h>

struct nspcb {
	LIST_ENTRY(nspcb) 	 	nsp_hash;
	LIST_ENTRY(nspcb) 	 	nsp_lhash;
	CIRCLEQ_ENTRY(nspcb) 		nsp_queue;
	struct nspcbtable		*nsp_table;
	int 				nsp_state;		/* bind/connect state */
	struct socket 			*nsp_socket;		/* back pointer to socket */
	struct ns_addr 			nsp_faddr;		/* destination address */
	struct ns_addr 			nsp_laddr;		/* socket's address */
	caddr_t				nsp_pcb;		/* protocol specific stuff */
	struct route 			nsp_route;		/* routing information */
	struct ns_addr 			nsp_lastdst;		/* validate cached route for dg socks*/
	long				nsp_notify_param;	/* extra info passed via ns_pcbnotify*/
	short				nsp_flags;
	u_int8_t 			nsp_dpt;		/* default packet type for idp_output*/
	u_int8_t 			nsp_rpt;		/* last received packet type by idp_input() */
};
#define nsp_lport 			nsp_laddr.x_port
#define nsp_fport 			nsp_faddr.x_port

LIST_HEAD(nspcbhead, nspcb);
CIRCLEQ_HEAD(nspcbqueue, nspcb);

struct nspcbtable {
	struct nspcbqueue 		nspt_queue;
	struct nspcbhead 		*nspt_hashtbl;
	struct nspcbhead 		*nspt_porthashtbl;
	struct nspcbhead 		*nspt_bindhashtbl;
	struct nspcbhead 		*nspt_connecthashtbl;
	u_long 				nspt_porthash;
	u_long 				nspt_bindhash;
	u_long 				nspt_connecthash;
};

/* states in nsp_state: */
#define	NSP_ATTACHED	0
#define	NSP_BOUND		1
#define	NSP_CONNECTED	2

static const struct	ns_addr zerons_addr;

#define	NSPCBHASH_PORT(table, lport) \
	&(table)->nspt_porthashtbl[ntohs(lport) & (table)->nspt_porthash]
#define	NSPCBHASH_BIND(table, laddr, lport) \
	&(table)->nspt_bindhashtbl[ \
	    ((ntohl((laddr).x_port) + ntohs(lport))) & (table)->nspt_bindhash]
#define	NSPCBHASH_CONNECT(table, faddr, fport, laddr, lport) \
	&(table)->nspt_connecthashtbl[ \
	    ((ntohl((faddr).x_port) + ntohs(fport)) + \
	     (ntohl((laddr).x_port) + ntohs(lport))) & (table)->nspt_connecthash]

struct nspcbhead *
ns_pcbhash(table, laddr, lport, faddr, fport)
	struct nspcbtable *table;
	struct ns_addr laddr, faddr;
	uint16_t lport, fport;
{
	struct nspcbhead *nshash;
	uint32_t nethash;

	uint32_t lahash = fnva_32_buf(laddr, sizeof(*laddr), FNV1_32_INIT);	/* laddr hash */
	uint32_t lphash = fnva_32_buf(&lport, sizeof(lport), FNV1_32_INIT);	/* lport hash */
	uint32_t fahash = fnva_32_buf(faddr, sizeof(*faddr), FNV1_32_INIT);	/* faddr hash */
	uint32_t fphash = fnva_32_buf(&fport, sizeof(fport), FNV1_32_INIT);	/* fport hash */

	nethash = ntohl(laddr.x_port) + ntohs(lport) + ntohl(faddr.x_port) + ntohs(fport);
	nshash = &table->nspt_hashtbl[nethash & table->nspt_hashtbl];
	return (nshash);
}

void
ns_pcbinit(table, bindhashsize, connecthashsize)
	struct nspcbtable *table;
	int bindhashsize, connecthashsize;
{
	CIRCLEQ_INIT(&table->nspt_queue);
	table->nspt_hashtbl = hashinit(bindhashsize, M_PCB, &table->nspt_hashtbl);
	table->nspt_bindhashtbl = hashinit(bindhashsize, M_PCB, &table->nspt_bindhash);
	table->nspt_connecthashtbl = hashinit(connecthashsize, M_PCB, &table->nspt_connecthash);
}

int
ns_pcballoc(so, v)
	struct socket *so;
	void *v;
{
	struct nspcbtable *table = v;
	struct nspcb *nsp;

	nsp = malloc(sizeof(*nsp), M_PCB, M_NOWAIT);
	if (nsp == 0) {
		return (ENOBUFS);
	}
	bzero((caddr_t)nsp, sizeof(*nsp));
	nsp->nsp_table = table;
	nsp->nsp_socket = so;
	so->so_pcb = nsp;
	ns_pcbhash(table, nsp->nsp_laddr, nsp->nsp_lport, nsp->nsp_faddr, nsp->nsp_lport);
	CIRCLEQ_INSERT_HEAD(&table->nspt_queue, nsp, nsp_queue);
	LIST_INSERT_HEAD(ns_pcbhash(table, nsp->nsp_laddr, nsp->nsp_lport, nsp->nsp_faddr, nsp->nsp_lport), nsp, nsp_lhash);
	ns_pcbstate(nsp, NSP_ATTACHED);
	return (0);
}

struct nspcb *
ns_pcblookup(table, faddr, fport_arg, laddr, lport_arg, wildp)
	struct nspcbtable *table;
	struct ns_addr faddr, laddr;
	u_int lport_arg, fport_arg;
	int wildp;
{
	struct nspcbhead *head;
	struct nspcb *nsp;
	int matchwild = 3, wildcard;
	u_int16_t lport = lport_arg;
	u_int16_t fport = fport_arg;

	head = ns_pcbhash(table, laddr, lport, faddr, fport);
	LIST_FOREACH(nsp, head, nsp_lhash) {
		if (nsp->nsp_lport != lport) {

		}
		if (ns_nullhost(nsp->nsp_faddr)) {
			if (!ns_nullhost(faddr))
				wildcard++;
		} else {
			if (ns_nullhost(*faddr))
				wildcard++;
			else {
				if (!ns_hosteq(nsp->nsp_faddr, faddr))
					continue;
				if (nsp->nsp_fport != fport) {
					if (nsp->nsp_fport != 0)
						continue;
					else
						wildcard++;
				}
			}
		}
	}

	head = ns_pcbhash(table, zerons_addr, lport, faddr, fport);
	LIST_FOREACH(nsp, head, nsp_hash) {

	}
}

int
ns_pcbbind(v, nam, p)
	void *v;
	struct mbuf *nam;
	struct proc *p;
{
	struct nspcb *nsp;
	struct socket *so;
	struct nspcbtable *table;
	struct nspcbhead *head;
	struct sockaddr_ns *sns;
	u_int16_t lport;
	int wild;

	nsp = v;
	so = nsp->nsp_socket;
	table = nsp->nsp_table;
	lport = 0;
	wild = 0;
	if (nsp->nsp_lport || !ns_nullhost(nsp->nsp_laddr)) {
		return (EINVAL);
	}
	if (nam == 0) {
		goto noname;
	}
	sns = mtod(nam, struct sockaddr_ns *);
	if (nam->m_len != sizeof(*sns)) {
		return (EINVAL);
	}
	if (!ns_nullhost(sns->sns_addr)) {
		int tport = sns->sns_port;

		sns->sns_port = 0; /* yech... */
		if (ifa_ifwithaddr(snstosa(sns)) == 0) {
			return (EADDRNOTAVAIL);
		}
		sns->sns_port = tport;
	}
	lport = sns->sns_port;
	if (lport) {
		struct nspcb *t;

		if (ntohs(lport) < NSPORT_RESERVED
				&& (p == 0 || suser1(p->p_ucred, &p->p_acflag))) {
			return (EACCES);
		}
		t = ns_pcblookup_port(table, &zerons_addr, lport, wild);
		if (t == 0) {
			return (EADDRINUSE);
		}
	}
	nsp->nsp_laddr = sns->sns_addr;
noname:
	if (lport == 0) {
		do {
			if (nspcb.nsp_lport++ < NSPORT_RESERVED) {
				nspcb.nsp_lport = NSPORT_RESERVED;
				lport = htons(nspcb.nsp_lport);
			}
		} while (ns_pcblookup_port(table, &zerons_addr, lport, wild));
	}
	nsp->nsp_lport = lport;
	LIST_REMOVE(&nsp->nsp_head, nsp_lhash);
	head = NSPCBHASH_PORT(table, nsp->nsp_lport);
	LIST_INSERT_HEAD(head, &nsp->nsp_head, nsp_lhash);
	ns_pcbstate(nsp, NSP_BOUND);
	return (0);
}

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sns.
 * If don't have a local address for this socket yet,
 * then pick one.
 */
int
ns_pcbconnect(v, nam)
	void *v;
	struct mbuf *nam;
{
	struct nspcb *nsp = v;
	struct ns_ifaddr *ia;
	struct sockaddr_ns *sns = mtod(nam, struct sockaddr_ns *);
	struct ns_addr *dst;
	struct route *ro;
	struct ifnet *ifp;

	if (nam->m_len != sizeof (*sns))
		return (EINVAL);
	if (sns->sns_family != AF_NS)
		return (EAFNOSUPPORT);
	if (sns->sns_port == 0 || ns_nullhost(sns->sns_addr))
		return (EADDRNOTAVAIL);
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
	ro = &nsp->nsp_route;
	dst = &satons_addr(ro->ro_dst);
	if (nsp->nsp_socket->so_options & SO_DONTROUTE)
		goto flush;
	if (!ns_neteq(nsp->nsp_lastdst, sns->sns_addr))
		goto flush;
	if (!ns_hosteq(nsp->nsp_lastdst, sns->sns_addr)) {
		if (ro->ro_rt && !(ro->ro_rt->rt_flags & RTF_HOST)) {
			/* can patch route to avoid rtalloc */
			*dst = sns->sns_addr;
		} else {
	flush:
			if (ro->ro_rt)
				RTFREE(ro->ro_rt);
			ro->ro_rt = (struct rtentry*) 0;
			nsp->nsp_laddr.x_net = ns_zeronet;
		}
	}/* else cached route is ok; do nothing */
	nsp->nsp_lastdst = sns->sns_addr;
	if ((nsp->nsp_socket->so_options & SO_DONTROUTE) == 0
			&& /*XXX*/
			(ro->ro_rt == (struct rtentry*) 0
					|| ro->ro_rt->rt_ifp == (struct ifnet*) 0)) {
		/* No route yet, so try to acquire one */
		ro->ro_dst.sa_family = AF_NS;
		ro->ro_dst.sa_len = sizeof(ro->ro_dst);
		*dst = sns->sns_addr;
		dst->x_port = 0;
		rtalloc(ro);
	}
	if (ns_neteqnn(nsp->nsp_laddr.x_net, ns_zeronet)) {
		/*
		 * If route is known or can be allocated now,
		 * our src addr is taken from the i/f, else punt.
		 */
		ia = 0;
		/*
		 * If we found a route, use the address
		 * corresponding to the outgoing interface
		 */
		if (ro->ro_rt && (ifp = ro->ro_rt->rt_ifp)) {
			TAILQ_FOREACH(ia, &ns_ifaddr, ia_list) {
				if (ia->ia_ifp == ifp) {
					break;
				}
			}
		}
		if (ia == 0) {
			u_int16_t fport = sns->sns_addr.x_port;
			sns->sns_addr.x_port = 0;
			ia = (struct ns_ifaddr *)ifa_ifwithdstaddr(snstosa(sns));
			sns->sns_addr.x_port = fport;
			if (ia == 0)
				ia = ns_iaonnetof(&sns->sns_addr);
			if (ia == 0)
				ia = TAILQ_FIRST(&ns_ifaddr);
			if (ia == 0)
				return (EADDRNOTAVAIL);
		}
		nsp->nsp_laddr.x_net = satons_addr(ia->ia_addr).x_net;
	}
	if (ns_pcblookup_connect(nsp->nsp_table, &sns->sns_addr,
			sns->sns_addr.x_port,
			!ns_nullhost(nsp->nsp_laddr) ? nsp->nsp_laddr : &ia->ia_addr,
			nsp->nsp_lport)) {
		return (EADDRINUSE);
	}
	if (ns_nullhost(nsp->nsp_laddr)) {
		if (nsp->nsp_lport == 0)
			(void) ns_pcbbind(nsp, (struct mbuf*) 0, (struct proc*) 0);
		nsp->nsp_laddr.x_host = ns_thishost;
	}
	nsp->nsp_faddr = sns->sns_addr;
	/* Includes nsp->nsp_fport = sns->sns_port; */
	return (0);
}

void
ns_pcbdisconnect(v)
	void *v;
{
	struct nspcb *nsp = v;

	nsp->nsp_faddr = zerons_addr;
	ns_pcbstate(nsp, NSP_BOUND);
	if (nsp->nsp_socket->so_state & SS_NOFDREF)
		ns_pcbdetach(nsp);
}

void
ns_pcbdetach(v)
	void *v;
{
	struct nspcb *nsp = v;
	struct socket *so = nsp->nsp_socket;

	so->so_pcb = 0;
	sofree(so);
	if (nsp->nsp_route.ro_rt)
		rtfree(nsp->nsp_route.ro_rt);
	ns_pcbstate(nsp, NSP_ATTACHED);
	LIST_REMOVE(&nsp->nsp_head, nsph_lhash);
	CIRCLEQ_REMOVE(&nsp->nsp_table->nspt_queue, &nsp->nsp_head, nsph_queue);
	free(nsp, M_PCB);
}

void
ns_setsockaddr(nsp, nam)
	struct nspcb *nsp;
	struct mbuf *nam;
{
	struct sockaddr_ns *sns = mtod(nam, struct sockaddr_ns *);

	nam->m_len = sizeof (*sns);
	sns = mtod(nam, struct sockaddr_ns *);
	bzero((caddr_t)sns, sizeof (*sns));
	sns->sns_len = sizeof(*sns);
	sns->sns_family = AF_NS;
	sns->sns_addr = nsp->nsp_laddr;
}

void
ns_setpeeraddr(nsp, nam)
	struct nspcb *nsp;
	struct mbuf *nam;
{
	struct sockaddr_ns *sns = mtod(nam, struct sockaddr_ns *);

	nam->m_len = sizeof (*sns);
	sns = mtod(nam, struct sockaddr_ns *);
	bzero((caddr_t)sns, sizeof (*sns));
	sns->sns_len = sizeof(*sns);
	sns->sns_family = AF_NS;
	sns->sns_addr  = nsp->nsp_faddr;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  Call the
 * protocol specific routine to handle each connection.
 * Also pass an extra parameter via the nspcb. (which may in fact
 * be a parameter list!)
 */
void
ns_pcbnotify(table, faddr, fport, laddr, lport, errno, notify, param)
	struct nspcbtable *table;
	struct ns_addr faddr, laddr;
	u_int16_t fport, lport;
	long param;
	int errno;
	void (*notify)(struct nspcb *);
{
	struct nspcbhead *head;
	struct nspcb *nsp, *oinp;
	int s = splnet();

	head = NSPCBHASH_CONNECT(table, faddr, fport, laddr, lport);
	for (nsp = (struct nspcb *)LIST_FIRST(head); nsp != NULL;) {
		if (!ns_hosteq(nsp->nsp_faddr, faddr)) {
		next:
			nsp = (struct nspcb *)LIST_NEXT(nsp, nsp_hash);
			continue;
		}
		if (nsp->nsp_socket == 0) {
			goto next;
		}
		if (errno) {
			nsp->nsp_socket->so_error = errno;
		}
		oinp = nsp;
		nsp = (struct nspcb *)LIST_NEXT(nsp, nsp_hash);
		oinp->nsp_notify_param = param;
		(*notify)(oinp);
	}
	splx(s);
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */
void
ns_rtchange(nsp)
	struct nspcb *nsp;
{
	if (nsp->nsp_route.ro_rt) {
		rtfree(nsp->nsp_route.ro_rt);
		nsp->nsp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
	/* SHOULD NOTIFY HIGHER-LEVEL PROTOCOLS */
}

struct nspcb *
ns_pcblookup_port(table, faddr, lport, wildp)
	struct nspcbtable *table;
	const struct ns_addr faddr;
	u_int16_t lport;
	int wildp;
{
	struct nspcbhead *head;
	struct nspcb_hdr *nsph;
	struct nspcb *nsp, *match;
	int matchwild = 3, wildcard;
	u_int16_t fport;

	fport = faddr->x_port;
	head = NSPCBHASH_PORT(table, lport);
	LIST_FOREACH(nsph, head, nsph_lhash) {
		nsp = (struct nspcb *)nsph;
		if (nsp->nsp_lport != lport) {
			continue;
		}
		wildcard = 0;
		if (ns_nullhost(nsp->nsp_faddr)) {
			if (!ns_nullhost(faddr))
				wildcard++;
		} else {
			if (ns_nullhost(*faddr))
				wildcard++;
			else {
				if (!ns_hosteq(nsp->nsp_faddr, faddr))
					continue;
				if (nsp->nsp_fport != fport) {
					if (nsp->nsp_fport != 0)
						continue;
					else
						wildcard++;
				}
			}
		}
		if (wildcard && wildp == 0)
			continue;
		if (wildcard < matchwild) {
			match = nsp;
			matchwild = wildcard;
			if (wildcard == 0)
				break;
		}
	}
	return (match);
}

struct nspcb *
ns_pcblookup_connect(table, faddr, fport, laddr, lport)
	struct nspcbtable *table;
	const struct ns_addr faddr, laddr;
	u_int16_t fport, lport;
{
	struct nspcbhead *head;
	struct nspcb_hdr *nsph;
	struct nspcb *nsp;

	head = NSPCBHASH_CONNECT(table, faddr, fport, laddr, lport);
	LIST_FOREACH(nsph, head, nsph_hash) {
		nsp = (struct nspcb *)nsph;
		if (ns_hosteq(nsp->nsp_faddr, faddr) &&
				nsp->nsp_fport == fport &&
				nsp->nsp_lport == lport &&
				ns_hosteq(nsp->nsp_laddr, laddr)) {
			goto out;
		}
	}

out:
	nsph = &nsp->nsp_head;
	if (nsph != LIST_FIRST(head)) {
		LIST_REMOVE(nsph, nsph_hash);
		LIST_INSERT_HEAD(head, nsph, nsph_hash);
	}
	return (nsp);
}

struct nspcb *
ns_pcblookup_bind(table, laddr, lport)
	struct nspcbtable *table;
	const struct ns_addr laddr;
	u_int16_t lport;
{
	struct nspcbhead *head;
	struct nspcb_hdr *nsph;
	struct nspcb *nsp;

	head = NSPCBHASH_BIND(table, laddr, lport);
	LIST_FOREACH(nsph, head, nsph_hash) {
		nsp = (struct nspcb *)nsph;
		if (nsp->nsp_lport == lport && ns_hosteq(nsp->nsp_laddr, laddr)) {
			goto out;
		}
	}
	head = NSPCBHASH_BIND(table, laddr, lport);
	LIST_FOREACH(nsph, head, nsph_hash) {
		nsp = (struct nspcb *)nsph;
		if (nsp->nsp_lport == lport && ns_hosteq(nsp->nsp_laddr, zerons_addr)) {
			goto out;
		}
	}

out:
	nsph = &nsp->nsp_head;
	if (nsph != LIST_FIRST(head)) {
		LIST_REMOVE(nsph, nsph_hash);
		LIST_INSERT_HEAD(head, nsph, nsph_hash);
	}
	return (nsp);
}

void
ns_pcbstate(nsp, state)
	const struct nspcb *nsp;
	int state;
{
	struct nspcbhead *head;

	if (nsp->nsp_state > NSP_ATTACHED) {
		LIST_REMOVE(&nsp->nsp_head, nsph_hash);
	}

	switch (state) {
	case NSP_BOUND:
		head = NSPCBHASH_BIND(nsp->nsp_table, nsp->nsp_laddr, nsp->nsp_lport);
		LIST_INSERT_HEAD(head, &nsp->nsp_head, nsph_hash);
		break;
	case NSP_CONNECTED:
		head = NSPCBHASH_CONNECT(nsp->nsp_table, nsp->nsp_faddr, nsp->nsp_fport, nsp->nsp_laddr, nsp->nsp_lport);
		LIST_INSERT_HEAD(head, &nsp->nsp_head, nsph_hash);
		break;
	}

	nsp->nsp_state = state;
}
