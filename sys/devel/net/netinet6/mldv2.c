/*	$KAME: mldv2.c,v 1.72 2007/06/14 12:09:44 itojun Exp $	*/
/*	$NetBSD: mld6.c,v 1.26 2004/03/28 08:28:50 christos Exp $	*/
/*	$KAME: mld6.c,v 1.25 2001/01/16 14:14:18 itojun Exp $	*/

/*
 * Copyright (c) 2002 INRIA. All rights reserved.
 *
 * Implementation of Multicast Listener Discovery, Version 2.
 * Developed by Hitoshi Asaeda, INRIA, August 2002.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of INRIA nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (C) 1998 WIDE Project.
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
 * Copyright (c) 1988 Stephen Deering.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Stephen Deering of Stanford University.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)igmp.c	8.1 (Berkeley) 7/19/93
 */

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/mld6_var.h>
#include <netinet6/in6_msf.h>
#include <netinet6/scope6_var.h>

#include <net/if_arp.h>
#include <net/if_ether.h>
#include <net/if_types.h>

#include <net/net_osdep.h>

/*
 * Protocol constants
 */

/* denotes that the MLD max response delay field specifies time in milliseconds */
#define MLD_TIMER_SCALE	1000

/*
 * time between repetitions of a node's initial report of interest in a
 * multicast address(in seconds)
 */
#define MLD_UNSOLICITED_REPORT_INTERVAL	10
#define MLDV2_UNSOLICITED_REPORT_INTERVAL 1

int mldmaxsrcfilter = IP_MAX_SOURCE_FILTER;
int mldsomaxsrc = SO_MAX_SOURCE_FILTER;

/*
 * mld_version:
 * 	0: mldv2 with compat mode
 * 	1: mldv1-only,
 * 	2: mldv2 without compat-mode
 */
#ifdef MLDV2
int mld_version = 0;
#else
int mld_verion = 1;
#endif

#ifdef MLDV2_DEBUG
int mld_debug = 1;
#else
int mld_debug = 0;
#endif

#ifdef MLDV2
static struct router6_info *head6;
#endif

static struct ip6_pktopts ip6_opts;
static int mld_group_timers_are_running;
#ifdef MLDV2
static int mld_state_change_timers_are_running;
static int mld_interface_timers_are_running;
#endif
/* XXX: These are necessary for KAME's link-local hack */
static struct in6_addr mld_all_nodes_linklocal = IN6ADDR_LINKLOCAL_ALLNODES_INIT;
static struct in6_addr mld_all_routers_linklocal = IN6ADDR_LINKLOCAL_ALLROUTERS_INIT;
#ifdef MLDV2
static struct in6_addr mld_all_nodes_nodelocal = IN6ADDR_NODELOCAL_ALLNODES_INIT;
static struct in6_addr mld_all_routersv2_linklocal = IN6ADDR_LINKLOCAL_ALLV2ROUTERS_INIT;

static const int qhdrlen = MLD_V2_QUERY_MINLEN;	/* mldv2 query header */
static const int rhdrlen = 8;	/* mldv2 report header */
static const int ghdrlen = 20;	/* mld group report header */
static const int addrlen = sizeof(struct in6_addr);

#define	SOURCE_RECORD_LEN(numsrc)	(numsrc * addrlen)

static struct router6_info *init_rt6i(struct ifnet *);
static struct router6_info *find_rt6i(struct ifnet *);

static int mld6_query_version(struct mbuf *, int, u_int16_t);
static int mld6_query_type(struct mldv2_hdr *, struct in6_addr *, int);

static int mld6_record_queried_source(struct in6_multi *, struct mld_hdr *, u_int16_t);
static void mld6_send_all_current_state_report(struct ifnet *);
static int mld6_send_current_state_report(struct in6_multi *);
static int mld6_create_group_record(struct mbuf *, struct in6_multi *, u_int16_t, u_int16_t *, u_int8_t);
static void mld6_cancel_pending_response(struct ifnet *, struct router6_info *);
static void mld6_sendbuf(struct mbuf *, struct ifnet *);

static void mld6_state_change_timeo(struct in6_multi *);
static void mld6_interface_timeo(struct in6_multi *, struct ifnet *);
#endif

static void mld6_group_timeo(struct in6_multi *);
static struct mld_hdr * mld6_allocbuf(struct mbuf **, int, struct in6_multi *, int);
static void mld6_sendpkt(struct in6_multi *, int, const struct in6_addr *);

void
mld6_init(void)
{
	static u_int8_t hbh_buf[8];
	struct ip6_hbh *hbh = (struct ip6_hbh *)hbh_buf;
	u_int16_t rtalert_code = htons((u_int16_t)IP6OPT_RTALERT_MLD);

	mld_group_timers_are_running = 0;
#ifdef MLDV2
	mld_state_change_timers_are_running = 0;
	mld_interface_timers_are_running = 0;
#endif

	/* ip6h_nxt will be fill in later */
	hbh->ip6h_len = 0;	/* (8 >> 3) - 1 */

	/* XXX: grotty hard coding... */
	hbh_buf[2] = IP6OPT_PADN;	/* 2 byte padding */
	hbh_buf[3] = 0;
	hbh_buf[4] = IP6OPT_RTALERT;
	hbh_buf[5] = IP6OPT_RTALERT_LEN - 2;
	bcopy((caddr_t)&rtalert_code, &hbh_buf[6], sizeof(u_int16_t));

	ip6_opts.ip6po_hbh = hbh;
	/* We will specify the hoplimit by a multicast option. */
	ip6_opts.ip6po_hlim = -1;

#ifdef MLDV2
	head6 = NULL;
#endif
}

#ifdef MLDV2
static struct router6_info *
init_rt6i(ifp)
	struct ifnet *ifp;
{
	struct router6_info *rti;

	MALLOC(rti, struct router6_info *, sizeof *rti, M_MRTABLE, M_NOWAIT);
	if (rti == NULL) {
		return (NULL);
	}
	rti->rt6i_ifp = ifp;
	rti->rt6i_timer1 = 0;
	rti->rt6i_timer2 = 0;
	rti->rt6i_qrv = MLD_DEF_RV;
	rti->rt6i_qqi = MLD_DEF_QI;
	rti->rt6i_qri = MLD_DEF_QRI / MLD_TIMER_SCALE;
	if (mld_version == 1) {
		rti->rt6i_type = MLD_V1_ROUTER;
	} else {
		rti->rt6i_type = MLD_V2_ROUTER;
	}
	rti->rt6i_next = head6;
	head6 = rti;
	return (rti);
}

static struct router6_info *
find_rt6i(struct ifnet *ifp)
{
	struct router6_info *rti = head6;

	while (rti) {
		if (rti->rt6i_ifp == ifp) {
			return (rti);
		}
		rti = rti->rt6i_next;
	}
	rti = init_rt6i(ifp);
	if (rti == NULL) {
		return (NULL);
	}
	return (rti);
}

#endif

#ifdef MLDV2

/* type - State-Change report type */
void
mld6_start_listening(in6m, type)
	struct in6_multi *in6m;
	u_int8_t type;
{
	struct in6_addr all_in6;
	struct in6_multi_source *i6ms;
	int s = splsoftnet();

	/*
	 * RFC2710 page 10:
	 * The node never sends a Report or Done for the link-scope all-nodes
	 * address.
	 * MLD messages are never sent for multicast addresses whose scope is 0
	 * (reserved) or 1 (node-local).
	 */
	all_in6 = mld_all_nodes_linklocal;
	all_in6.s6_addr16[1] = htons(in6m->in6m_ifp->if_index); /* XXX */
	if (in6_setscope(&all_in6, in6m->in6m_ifp, NULL)) {
		/* XXX: this should not happen! */
		in6m->in6m_timer = 0;
		in6m->in6m_state = MLD_OTHERLISTENER;
	}
	if (IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &all_in6) ||
	    IPV6_ADDR_MC_SCOPE(&in6m->in6m_addr) < IPV6_ADDR_SCOPE_LINKLOCAL) {
		in6m->in6m_timer = 0;
		in6m->in6m_state = MLD_OTHERLISTENER;
	} else {
		/* Current-State Record timer */
		if (in6m->in6m_rti->rt6i_type == MLD_V2_ROUTER) {
			mld6_send_state_change_report(in6m, type, 1);
			i6ms = in6m->in6m_source;
			if (i6ms != NULL) {
				i6ms->i6ms_timer =
						MLD_RANDOM_DELAY(MLD_UNSOLICITED_REPORT_INTERVAL * PR_FASTHZ);
				mld_state_change_timers_are_running = 1;
			}
		} else {
			mld6_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
			in6m->in6m_timer =
			    MLD_RANDOM_DELAY(MLD_UNSOLICITED_REPORT_INTERVAL *
			    PR_FASTHZ);
			in6m->in6m_state = MLD_IREPORTEDLAST;
			mld_group_timers_are_running = 1;
		}

	}
	splx(s);
}

#else

void
mld6_start_listening(in6m)
	struct in6_multi *in6m;
{
	struct in6_addr all_in6;
	int s = splsoftnet();

	/*
	 * RFC2710 page 10:
	 * The node never sends a Report or Done for the link-scope all-nodes
	 * address.
	 * MLD messages are never sent for multicast addresses whose scope is 0
	 * (reserved) or 1 (node-local).
	 */
	all_in6 = mld_all_nodes_linklocal;
	all_in6.s6_addr16[1] = htons(in6m->in6m_ifp->if_index); /* XXX */
	if (in6_setscope(&all_in6, in6m->in6m_ifp, NULL)) {
		/* XXX: this should not happen! */
		in6m->in6m_timer = 0;
		in6m->in6m_state = MLD_OTHERLISTENER;
	}
	if (IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &all_in6) ||
	    IPV6_ADDR_MC_SCOPE(&in6m->in6m_addr) < IPV6_ADDR_SCOPE_LINKLOCAL) {
		in6m->in6m_timer = 0;
		in6m->in6m_state = MLD_OTHERLISTENER;
	} else {
		mld6_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
		in6m->in6m_timer =
		    MLD_RANDOM_DELAY(MLD_UNSOLICITED_REPORT_INTERVAL *
		    PR_FASTHZ);
		in6m->in6m_state = MLD_IREPORTEDLAST;
		mld_group_timers_are_running = 1;
	}
	splx(s);
}

#endif

void
mld6_stop_listening(in6m)
	struct in6_multi *in6m;
{
	struct in6_addr allnode, allrouter;

	allnode = mld_all_nodes_linklocal;
	allnode.s6_addr16[1] = htons(in6m->in6m_ifp->if_index); /* XXX */
	if (in6_setscope(&allnode, in6m->in6m_ifp, NULL)) {
		/* XXX: this should not happen! */
		return;
	}

	allrouter = mld_all_routers_linklocal;
	allrouter.s6_addr16[1] = htons(in6m->in6m_ifp->if_index); /* XXX: necessary when mrouting */
	if (in6_setscope(&allrouter, in6m->in6m_ifp, NULL)) {
		/* XXX impossible */
		return;
	}

	if (in6m->in6m_state == MLD_IREPORTEDLAST &&
	    (!IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &allnode)) &&
	    IPV6_ADDR_MC_SCOPE(&in6m->in6m_addr) > IPV6_ADDR_SCOPE_NODELOCAL) {
		mld6_sendpkt(in6m, MLD_LISTENER_DONE,
		    &allrouter);
	}
}

#ifdef MLDV2

static int
mld6_query_version(m, off, mldlen)
	struct mbuf *m;
	int off;
	u_int16_t mldlen;
{
	int query_ver = 0;

	if (mldlen > MLD_MINLEN && mldlen < MLD_V2_QUERY_MINLEN) {
		mldlog((LOG_DEBUG, "invalid MLD packet(len=%d)\n", mldlen));
	} else if (mldlen == MLD_MINLEN || mld_version == 1) {
		query_ver = MLD_V1_ROUTER;
	} else {
		/* MLDv2 Query: fall back to MLDv1 Query, if necessary */
		query_ver = MLD_V2_ROUTER;
	}
	return (query_ver);
}

static int
mld6_query_type(mldv2h, mld_addr, query_ver)
	struct mldv2_hdr *mldv2h;
	struct in6_addr *mld_addr;
	int query_ver;
{
	int query_type = 0;

	/* no buffer-overrun here, since mldlen >= MLD_V2_QUERY_MINLEN */
	if (query_ver != MLD_V2_ROUTER) {
		return (query_type);
	}
	if (IN6_IS_ADDR_MULTICAST(mld_addr)) {
		if (mldv2h->mld_numsrc != 0) {
			mldlog((LOG_DEBUG, "invalid general query(numsrc=%d)\n",
					mldv2h->mld_numsrc));
		}
		query_type = MLD_V2_GENERAL_QUERY;
		mldlog((LOG_DEBUG, "MLDv2 general Query\n"));
	}
	if (IN6_IS_ADDR_MULTICAST(mld_addr)) {
		if (mldv2h->mld_numsrc == 0) {
			query_type = MLD_V2_GROUP_QUERY;
			mldlog((LOG_DEBUG, "MLDv2 group Query\n"));
		} else {
			query_type = MLD_V2_GROUP_SOURCE_QUERY;
			mldlog((LOG_DEBUG, "MLDv2 source-group Query\n"));
		}
	}
	return (query_type);
}

#endif

void
mld6_input(m, off)
	struct mbuf *m;
	int off;
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct mld_hdr *mldh;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct in6_multi *in6m;
	struct in6_addr mld_addr, all_in6;
	struct in6_ifaddr *ia;
#ifdef MLDV2
	struct mldv2_hdr *mldv2h;
	struct router6_info *rt6i;
	int query_type, query_ver;
	u_int16_t mldlen;
	int timer = 0;
#else
	int timer;
#endif

	IP6_EXTHDR_GET(mldh, struct mld_hdr *, m, off, sizeof(*mldh));
	if (mldh == NULL) {
		icmp6stat.icp6s_tooshort++;
		return;
	}

	/* source address validation */
	ip6 = mtod(m, struct ip6_hdr *);/* in case mpullup */
	if (!IN6_IS_ADDR_LINKLOCAL(&ip6->ip6_src)) {
		/*
		 * RFC3590 allows the IPv6 unspecified address as the source
		 * address of MLD report and done messages.  However, as this
		 * same document says, this special rule is for snooping
		 * switches and the RFC requires routers to discard MLD packets
		 * with the unspecified source address.  The RFC only talks
		 * about hosts receiving an MLD query or report in Security
		 * Considerations, but this is probably the correct intention.
		 * RFC3590 does not talk about other cases than link-local and
		 * the unspecified source addresses, but we believe the same
		 * rule should be applied.
		 * As a result, we only allow link-local addresses as the
		 * source address; otherwise, simply discard the packet.
		 */
#if 0
		log(LOG_ERR,
		    "mld_input: src %s is not link-local (grp=%s)\n",
		    ip6_sprintf(&ip6->ip6_src),
		    ip6_sprintf(&mldh->mld_addr));
#endif
		/*
		 * spec (RFC2710) does not explicitly
		 * specify to discard the packet from a non link-local
		 * source address. But we believe it's expected to do so.
		 * XXX: do we have to allow :: as source?
		 */
		m_freem(m);
		return;
	}

	/*
	 * make a copy for local work (in6_setscope() may modify the 1st arg)
	 */
	mld_addr = mldh->mld_addr;
	if (in6_setscope(&mld_addr, ifp, NULL)) {
		/* XXX: this should not happen! */
		m_free(m);
		return;
	}

#ifdef MLDV2

	rt6i = find_rt6i(ifp);
	if (rt6i == NULL) {
		mldlog((LOG_DEBUG,
			"mld_input(): cannot find router6_info at link#%d\n",
			ifp->if_index));
		m_free(m);
		return;
	}
#endif

	/*
	 * In the MLD6 specification, there are 3 states and a flag.
	 *
	 * In Non-Listener state, we simply don't have a membership record.
	 * In Delaying Listener state, our timer is running (in6m->in6m_timer)
	 * In Idle Listener state, our timer is not running (in6m->in6m_timer==0)
	 *
	 * The flag is in6m->in6m_state, it is set to MLD_OTHERLISTENER if
	 * we have heard a report from another member, or MLD_IREPORTEDLAST
	 * if we sent the last report.
	 */
	switch (mldh->mld_type) {
	case MLD_LISTENER_QUERY:
		if (ifp->if_flags & IFF_LOOPBACK)
			break;

		if (!IN6_IS_ADDR_UNSPECIFIED(&mld_addr)
				&& !IN6_IS_ADDR_MULTICAST(&mld_addr)) {
			break; /* print error or log stat? */
		}

		all_in6 = mld_all_nodes_linklocal;
		if (in6_setscope(&all_in6, ifp, NULL)) {
			/* XXX: this should not happen! */
			break;
		}

		if (IN6_IS_ADDR_MC_LINKLOCAL(&mld_addr)) {
			mldh->mld_addr.s6_addr16[1] = htons(ifp->if_index); /* XXX */
		}

#ifdef MLDV2

		/*
		 * MLD version and Query type check.
		 * MLDv1 Query: length = 24 octets AND Max-Resp-Code = 0
		 * MLDv2 Query: length >= 28 octets AND Max-Resp-Code != 0
		 * (MLDv1 implementation must accept only the first 24
		 * octets of the query message)
		 *
		 * If sysctl variable "mld_version" is specified to 1,
		 * query-type will be MLDv1 regardless of the packet size.
		 */
		mldlen = m->m_pkthdr.len - off;
		query_ver = mld6_query_version(m, off, mldlen);
		if (query_ver != MLD_V2_ROUTER) {
			break;
		}
		/* no buffer-overrun here, since mldlen >= MLD_V2_QUERY_MINLEN */
		mldv2h = (struct mldv2_hdr *)mldh;
		query_type = mld6_query_type(mldv2h, &mld_addr, query_ver);
		if (query_type != 0) {
			if (rt6i->rt6i_type != MLD_V1_ROUTER) {
				break;
			}
		} else {
			if (mld6_set_timer(ifp, rt6i, mldh, mldlen, query_type) != 0) {
				break;
			}
		}
#endif
		/*
		 * - Start the timers in all of our membership records
		 *   that the query applies to for the interface on
		 *   which the query arrived excl. those that belong
		 *   to the "all-nodes" group (ff02::1).
		 * - Restart any timer that is already running but has
		 *   A value longer than the requested timeout.
		 * - Use the value specified in the query message as
		 *   the maximum timeout.
		 */
		IFP_TO_IA6(ifp, ia);
		if (ia == NULL) {
			break;
		}

		timer = MLD_FASTTIMER(ntohs(mldh->mld_maxdelay));
		if (timer == 0 && mldh->mld_maxdelay) {
			timer = 1;
		}
		mld_all_nodes_linklocal.s6_addr16[1] = htons(ifp->if_index); /* XXX */

		for (in6m = LIST_FIRST(&ia->ia6_multiaddrs);
				in6m;
				in6m = LIST_NEXT(in6m, in6m_entry))
		{
			if (IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &mld_all_nodes_linklocal)
					|| IPV6_ADDR_MC_SCOPE(&in6m->in6m_addr)
							< IPV6_ADDR_SCOPE_LINKLOCAL) {
				continue;
			}
			if (IN6_IS_ADDR_UNSPECIFIED(&mld_addr)
					|| IN6_ARE_ADDR_EQUAL(&mld_addr, &in6m->in6m_addr)) {
				if (timer == 0) {
					/* send a report immediately */
					mld6_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
					in6m->in6m_timer = 0; /* reset timer */
					in6m->in6m_state = MLD_IREPORTEDLAST;
				} else if (in6m->in6m_timer == 0 || /*idle state*/
						in6m->in6m_timer > timer) {
					in6m->in6m_timer = MLD_RANDOM_DELAY(timer);
					mld_group_timers_are_running = 1;
				}
			}
		}
		if (IN6_IS_ADDR_MC_LINKLOCAL(&mld_addr)) {
			mld_addr.s6_addr16[1] = 0; /* XXX */
		}
		break;

	case MLD_LISTENER_REPORT:
		/*
		 * For fast leave to work, we have to know that we are the
		 * last person to send a report for this group.  Reports
		 * can potentially get looped back if we are a multicast
		 * router, so discard reports sourced by me.
		 * Note that it is impossible to check IFF_LOOPBACK flag of
		 * ifp for this purpose, since ip6_mloopback pass the physical
		 * interface to looutput.
		 */
		if (m->m_flags & M_LOOP) {
			break;
		}

		/* MLDv1/v2 Query */
		if (ifp->if_flags & IFF_LOOPBACK)
			break;

		if (!IN6_IS_ADDR_UNSPECIFIED(&mldh->mld_addr)
				&& !IN6_IS_ADDR_MULTICAST(&mldh->mld_addr)) {
			break; /* print error or log stat? */
		}

		/*
		 * If we belong to the group being reported, stop
		 * our timer for that group.
		 */
		IN6_LOOKUP_MULTI(mld_addr, ifp, in6m);
		if (in6m) {
			/* transit to idle state */
			in6m->in6m_timer = 0;
			in6m->in6m_state = MLD_OTHERLISTENER; /* clear flag */
		}
		if (IN6_IS_ADDR_MC_LINKLOCAL(&mld_addr)) {
			mld_addr.s6_addr16[1] = 0; /* XXX */
		}
		break;
	default:
#if 0
		/*
		 * this case should be impossible because of filtering in
		 * icmp6_input().  But we explicitly disabled this part
		 * just in case.
		 */
		log(LOG_ERR, "mld6_input: illegal type(%d)", mldh->mld_type);
#endif
		break;
	}
#ifdef MLDV2
	/*
	 * MLDv1 Querier Present is set to Older Version Querier Present
	 * Timeout seconds whenever an MLDv1 General Query is received.
	 */
	if (query_ver == MLD_V1_ROUTER
			&& IN6_ARE_ADDR_EQUAL(&mld_addr, &in6addr_any)) {
		if (mld_version == 2) {
			m_freem(m);
			return;
		}
		rt6i->rt6i_timer1 = rt6i->rt6i_qrv * rt6i->rt6i_qqi + rt6i->rt6i_qri;
		rt6i->rt6i_timer1 *= PR_FASTHZ;
		mld_interface_timers_are_running = 1;
	}
	rt6i->rt6i_type = MLD_V1_ROUTER;
#endif
	m_freem(m);
}

#ifdef MLDV2
/*
 * Timer adjustment on reception of an MLDv2 Query.
 */
static int
mld6_set_timer(ifp, rti, mld, mldlen, query_type)
	struct ifnet *ifp;
	struct router6_info *rti;
	struct mld_hdr *mld;
	u_int16_t mldlen;
	u_int8_t query_type;
{
	struct in6_multi *in6m;
	struct in6_multistep step;
	struct mldv2_hdr *mldh = (struct mldv2_hdr *)mld;
	int timer;			/* Max-Resp-Timer */
	int timer_i = 0;		/* interface timer */
	int timer_g = 0;		/* group timer */
	int error;
#if defined(__NetBSD__) || defined(__OpenBSD__)
	int s = splsoftnet();
#else
	int s = splnet();
#endif

	/*
	 * Parse QRV, QQI, and QRI timer values.
	 */
	if (((rti->rt6i_qrv = MLD_QRV(mldh->mld_rtval)) == 0) ||
	    (rti->rt6i_qrv > 7)) {
		rti->rt6i_qrv = MLD_DEF_RV;
	}

	if ((mldh->mld_qqi > 0) && (mldh->mld_qqi < 128)) {
		rti->rt6i_qqi = mldh->mld_qqi;
	} else if (mldh->mld_qqi >= 128) {
		rti->rt6i_qqi = ((MLD_QQIC_MANT(mldh->mld_qqi) | 0x10)
				<< (MLD_QQIC_EXP(mldh->mld_qqi) + 3));
	} else {
		rti->rt6i_qqi = MLD_DEF_QI;
	}
	rti->rt6i_qri = ntohs(mldh->mld_maxdelay);
	if (rti->rt6i_qri >= rti->rt6i_qqi * MLD_TIMER_SCALE) {
		rti->rt6i_qri = (rti->rt6i_qqi - 1);
		/* XXX tentatively adjusted */
	} else {
		rti->rt6i_qri /= MLD_TIMER_SCALE;
	}

	if (ntohs(mldh->mld_maxdelay) == 0) {
		/*
		 * XXX: this interval prevents an MLD-Report flooding caused
		 * by an MLD-query with Max-Reponse-Delay=0 (KAME local design)
		 */
		timer = MLD_TIMER_SCALE;
	} else if (ntohs(mldh->mld_maxdelay) < 32768) {
		timer = ntohs(mldh->mld_maxdelay);
	} else {
		timer = (MLD_MRC_MANT(mldh->mld_maxdelay) | 0x1000)
				<< (MLD_MRC_EXP(mldh->mld_maxdelay) + 3);
	}
	mldlog((LOG_DEBUG, "mld6_set_timer: qrv=%d,qqi=%d,qri=%d,timer=%d\n",
		rti->rt6i_qrv, rti->rt6i_qqi, rti->rt6i_qri, timer));

	/*
	 * Set interface timer if the query is Generic Query.
	 * Get group timer if the query is not Generic Query.
	 */
	if (query_type == MLD_V2_GENERAL_QUERY) {
		timer_i = MLD_FASTTIMER(timer);
		if (timer_i == 0) {
			timer_i = PR_FASTHZ;
		}
		timer_i = MLD_RANDOM_DELAY(timer_i);
		if (rti->rt6i_timer2 != 0 && rti->rt6i_timer2 < timer_i) {
			mldlog((LOG_DEBUG, "mld6_set_timer: don't do anything "
			    "as appropriate I/F timer (%d) is already running"
			    "(planned=%d)\n",
			    rti->rt6i_timer2 / PR_FASTHZ, timer_i / PR_FASTHZ));
			/* don't need to update interface timer */
		} else {
			mldlog((LOG_DEBUG, "mld6_set_timer: set I/F timer "
			    "to %d\n", timer_i / PR_FASTHZ));
			rti->rt6i_timer2 = timer_i;
			mld_interface_timers_are_running = 1;
		}
	} else { /* G or SG query */
		timer_g = MLD_FASTTIMER(timer);
		if (timer_g == 0) {
			timer_g = PR_FASTHZ;
		}
		timer_g = MLD_RANDOM_DELAY(timer_g);
		mldlog((LOG_DEBUG,
		    "mld6_set_timer: set group timer to %d\n", timer_g / PR_FASTHZ));
	}

	IN6_FIRST_MULTI(step, in6m);
	while (in6m != NULL) {
		if (!in6_is_mld_target(&in6m->in6m_addr) ||
		    in6m->in6m_ifp != ifp) {
			goto next_multi;
		}

		if ((in6m->in6m_source->i6ms_grpjoin == 0) &&
		    (in6m->in6m_source->i6ms_mode == MCAST_INCLUDE) &&
		    (in6m->in6m_source->i6ms_cur->numsrc == 0)) {
			goto next_multi; /* no need to consider any timer */
		}

		if (query_type == MLD_V2_GENERAL_QUERY) {
			/* Any previously pending response to Group- or
			 * Group-and-Source-Specific Query is canceled, if
			 * pending group timer is not sooner than new
			 * interface timer.
			 */
			if (in6m->in6m_timer <= rti->rt6i_timer2) {
				goto next_multi;
			}
			if ((in6m->in6m_source->i6ms_grpjoin == 0) &&
			    (in6m->in6m_source->i6ms_mode == MCAST_INCLUDE) &&
			    (in6m->in6m_source->i6ms_cur->numsrc == 0)) {
				goto next_multi; /* no need to consider any timer */
			}
			mldlog((LOG_DEBUG,
			    "mld6_set_timer: clears pending response\n"));
			in6m->in6m_state = MLD_OTHERLISTENER;
			in6m->in6m_timer = 0;
			in6_free_msf_source_list(in6m->in6m_source->i6ms_rec->head);
			in6m->in6m_source->i6ms_rec->numsrc = 0;
			goto next_multi;
		} else if (!IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr,
			    &mldh->mld_addr)) {
			goto next_multi;
		}

		/*
		 * If interface timer is sooner than new group timer,
		 * just ignore this Query for this group address.
		 */
		if (rti->rt6i_timer2 < timer_g) {
			in6m->in6m_state = MLD_OTHERLISTENER;
			in6m->in6m_timer = 0;
			break;
		}

		/* Receive Group-Specific Query */
		if (query_type == MLD_V2_GROUP_QUERY) {
			/*
			 * Group-Source list is cleared and a single response is
			 * scheduled, and group timer is set the earliest of the
			 * remaining time for the pending report and the
			 * selected delay.
			 */
			if ((in6m->in6m_state != MLD_G_QUERY_PENDING_MEMBER) &&
			    (in6m->in6m_state != MLD_SG_QUERY_PENDING_MEMBER)) {
				in6m->in6m_timer = timer_g;
				mld_group_timers_are_running = 1;
			} else {
				in6_free_msf_source_list(in6m->in6m_source->i6ms_rec->head);
				in6m->in6m_source->i6ms_rec->numsrc = 0;
				in6m->in6m_timer =
				    min(in6m->in6m_timer, timer_g);
			}
			in6m->in6m_state = MLD_G_QUERY_PENDING_MEMBER;
			break;
		}

		/* Receive Group-and-Source-Specific Query */
		if (in6m->in6m_state == MLD_G_QUERY_PENDING_MEMBER) {
			/*
			 * If there is a pending response for this group's
			 * Group-Specific Query, then queried sources are not
			 * recorded and pending status is not changed. Only the
			 * timer may be changed.
			 */
			in6m->in6m_timer = min(in6m->in6m_timer, timer_g);
			break;
		}
		/* Queried sources are augmented. */
		error = mld6_record_queried_source(in6m, mld, mldlen);
		if (error > 0) {
			/* XXX: ToDo: ICMPv6 error statistics */
			splx(s);
			return error;
		}
		if (error < 0) {
			break;	/* no need to do any additional things */
		}
		in6m->in6m_state = MLD_SG_QUERY_PENDING_MEMBER;
		if (in6m->in6m_timer != 0) {
			in6m->in6m_timer = min(in6m->in6m_timer, timer_g);
		} else {
			in6m->in6m_timer = timer_g;
		}
		mld_group_timers_are_running = 1;
		break;

next_multi:
		IN6_NEXT_MULTI(step, in6m);
	}
	splx(s);
	return 0;
}
#endif

void
mld6_fastimeo(void)
{
	struct in6_multi *in6m;
	struct in6_multistep step;
	int s;

	/*
	 * Quick check to see if any work needs to be done, in order
	 * to minimize the overhead of fasttimo processing.
	 */
#ifdef MLDV2
	if (!mld_group_timers_are_running && !mld_interface_timers_are_running && !mld_state_change_timers_are_running) {
		return;
	}
#else
	if (!mld_group_timers_are_running) {
		return;
	}
#endif

	s = splsoftnet();
	mld_group_timers_are_running = 0;
#ifdef MLDV2
	mld_interface_timers_are_running = 0;
	mld_state_change_timers_are_running = 0;
#endif
	IN6_FIRST_MULTI(step, in6m);
	while (in6m != NULL) {
#ifdef MLDV2
		mld6_state_change_timeo(in6m);
		mld6_interface_timeo(in6m, in6m->in6m_ifp);
#endif
		mld6_group_timeo(in6m);
		IN6_NEXT_MULTI(step, in6m);
	}
	splx(s);
}

static void
mld6_group_timeo(in6m)
	struct in6_multi *in6m;
{
	if (in6m->in6m_timer == 0) {
		/* do nothing */
	} else if (--in6m->in6m_timer == 0) {
#ifdef MLDV2
		if (in6m->in6m_rti->rt6i_type == MLD_V1_ROUTER) {
			mld6_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
			in6m->in6m_state = MLD_IREPORTEDLAST;
		}
		if (in6m->in6m_state == MLD_G_QUERY_PENDING_MEMBER
				|| in6m->in6m_state == MLD_SG_QUERY_PENDING_MEMBER) {
			mld6_send_current_state_report(in6m);
			in6m->in6m_state = MLD_OTHERLISTENER;
		}
#else
		mld6_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
		in6m->in6m_state = MLD_IREPORTEDLAST;
#endif
	} else {
		mld_group_timers_are_running = 1;
	}
}

#ifdef MLDV2

static void
mld6_state_change_timeo(in6m)
	struct in6_multi *in6m;
{
	struct in6_multi_source *i6ms;

	i6ms = in6m->in6m_source;
	if (i6ms != NULL) {
		if (i6ms->i6ms_timer == 0 && i6ms->i6ms_robvar == 0) {
			/* do nothing */
		} else if (--i6ms->i6ms_timer == 0) {
			/*
			 * Check if this report was pending Source-List-Change report or not.
			 * It is only the case that robvar was not reduced here.
			 * (XXX rarely, QRV may be changed in a same timing.)
			 */
			if (i6ms->i6ms_robvar == in6m->in6m_rti->rt6i_qrv) {
				mld6_send_state_change_report(in6m, 0, 1);
			} else if (i6ms->i6ms_robvar > 0) {
				mld6_send_state_change_report(in6m, 0, 0);
			}
		} else {
			/* schedule the next state change report */
			mld_state_change_timers_are_running = 1;
		}
	}
}

static void
mld6_interface_timeo(in6m, ifp)
	struct in6_multi *in6m;
	struct ifnet *ifp;
{
	struct router6_info *rti;

	rti = find_rt6i(ifp);
	if (rti != NULL) {
		if (rti->rt6i_timer1 == 0) {
			if (mld_version == 1) {
				if (rti->rt6i_type != MLD_V1_ROUTER) {
					rti->rt6i_type = MLD_V1_ROUTER;
				} else {
					if (rti->rt6i_type != MLD_V2_ROUTER) {
						rti->rt6i_type = MLD_V2_ROUTER;
					}
				}
			}
		} else if (--rti->rt6i_timer1 == 0) {
			switch (rti->rt6i_type) {
			case MLD_V1_ROUTER:
				/* Keep Older Version Querier Present timer */
				break;
			case MLD_V2_ROUTER:
				/*
				 * Check/set host compatibility mode. Whenever a host changes
				 * its compatability mode, cancel all its pending response and
				 * retransmission timers.
				 */
				mld6_cancel_pending_response(ifp, rti);
				break;
			default:
				/* impossible */
				break;
			}
		} else {
			goto running;
		}

		if (rti->rt6i_timer2 == 0) {
			/* do nothing */
		} else if (--rti->rt6i_timer2 == 0) {
			mld6_send_all_current_state_report(ifp);
		} else {
running:
			mld_interface_timers_are_running = 1;
		}
	}
}

#endif

static struct mld_hdr *
mld6_allocbuf(mh, len, in6m, type)
	struct mbuf **mh;
	int len, type;
	struct in6_multi *in6m;
{
	struct mbuf *md;
	struct mld_hdr *mldh;
	struct ip6_hdr *ip6;

	/*
	 * Allocate mbufs to store ip6 header and MLD header.
	 * We allocate 2 mbufs and make chain in advance because
	 * it is more convenient when inserting the hop-by-hop option later.
	 */
	MGETHDR(*mh, M_DONTWAIT, MT_HEADER);
	if (mh == NULL)
		return NULL;
	MGET(md, M_DONTWAIT, MT_DATA);

	/* uses cluster in case of MLDv2 */
	if (md && (len > MLD_MINLEN || type == MLDV2_LISTENER_REPORT)) {
		/* XXX: assumes len is less than 2K Byte */
		MCLGET(md, M_DONTWAIT);
		if ((md->m_flags & M_EXT) == 0) {
			m_free(md);
			md = NULL;
		}
	}
	if (md == NULL) {
		m_free(mh);
		return NULL;
	}
	(*mh)->m_next = md;
	md->m_next = NULL;

	(*mh)->m_pkthdr.rcvif = NULL;
	(*mh)->m_pkthdr.len = sizeof(struct ip6_hdr) + len;
	(*mh)->m_len = sizeof(struct ip6_hdr);
	MH_ALIGN(*mh, sizeof(struct ip6_hdr));

	/* fill in the ip6 header */
	ip6 = mtod(*mh, struct ip6_hdr *);
	ip6->ip6_flow = 0;
	ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc |= IPV6_VERSION;
	/* ip6_plen will be set later */
	ip6->ip6_nxt = IPPROTO_ICMPV6;
	/* ip6_hlim will be set by im6o.im6o_multicast_hlim */
	/* ip6_src/dst will be set by mld_sendpkt() or mld_sendbuf() */

	/* fill in the MLD header as much as possible */
	md->m_len = len;
	mldh = mtod(md, struct mld_hdr *);
	bzero(mldh, len);
	mldh->mld_type = type;
	return mldh;
}

#ifdef MLDV2

static void
mld6_sendbuf(mh, ifp)
	struct mbuf *mh;
	struct ifnet *ifp;
{
	struct ip6_hdr *ip6;
	struct mld_report_hdr *mld_rhdr;
	struct mld_group_record_hdr *mld_ghdr;
	int len;
	u_int16_t i;
	struct ip6_moptions im6o;
	struct mbuf *md;
	struct in6_ifaddr *ia = NULL;
	int ignflags;

	/*
	 * At first, find a link local address on the outgoing interface
	 * to use as the source address of the MLD packet.
	 * We do not reject tentative addresses for MLD report to deal with
	 * the case where we first join a link-local address.
	 */
	ignflags = (IN6_IFF_NOTREADY|IN6_IFF_ANYCAST) & ~IN6_IFF_TENTATIVE;
	if ((ia = in6ifa_ifpforlinklocal(ifp, ignflags)) == NULL)
		return;
	if ((ia->ia6_flags & IN6_IFF_TENTATIVE))
		ia = NULL;

	/*
	 * assumes IPv6 header and MLD header are located in the 1st and
	 * 2nd mbuf respecitively. (done in mld_allocbuf())
	 */
	if (mh == NULL) {
		mldlog((LOG_DEBUG, "mld6_sendbuf: mbuf is NULL\n"));
		return;
	}
	md = mh->m_next;

	/* fill src/dst here */
	ip6 = mtod(mh, struct ip6_hdr *);
	ip6->ip6_src = ia != NULL ? ia->ia_addr.sin6_addr : in6addr_any;
	ip6->ip6_dst = mld_all_routersv2_linklocal;
	 /*
	  * XXX: it should be impossible to fail at these functions here,
	  * but check the return value for sanity
	  */
	if (in6_setscope(&ip6->ip6_src, ifp, NULL) ||
	    in6_setscope(&ip6->ip6_dst, ifp, NULL))
		return;

	mld_rhdr = mtod(md, struct mld_report_hdr *);
	len = sizeof(struct mld_report_hdr);
	for (i = 0; i < mld_rhdr->mld_grpnum; i++) {
		mld_ghdr = (struct mld_group_record_hdr *)
					((char *)mld_rhdr + len);
		mldlog((LOG_DEBUG, "  %s\n", ip6_sprintf(&mld_ghdr->group)));
		len += ghdrlen + SOURCE_RECORD_LEN(mld_ghdr->numsrc);
		mld_ghdr->numsrc = htons(mld_ghdr->numsrc);
	}
	mld_rhdr->mld_grpnum = htons(mld_rhdr->mld_grpnum);
	mld_rhdr->mld_cksum = 0;

	mld_rhdr->mld_cksum = in6_cksum(mh, IPPROTO_ICMPV6, sizeof(struct ip6_hdr),
			len);
	im6o.im6o_multicast_ifp = ifp;
	im6o.im6o_multicast_hlim = ip6_defmcasthlim;
	im6o.im6o_multicast_loop = (ip6_mrouter != NULL);

	/* XXX: ToDo: create MLDv2 statistics field */
	//icmp6_ifstat_inc(ifp, ifs6_out_mldreport);
	ip6_output(mh, &ip6_opts, NULL, ia != NULL ? 0 : IPV6_UNSPECSRC, &im6o,
			NULL, NULL);
}

#endif

static void
mld6_sendpkt(in6m, type, dst)
	struct in6_multi *in6m;
	int type;
	const struct in6_addr *dst;
{
	struct mbuf *mh, *md;
	struct mld_hdr *mldh;
	struct ip6_hdr *ip6;
	struct ip6_moptions im6o;
	struct in6_ifaddr *ia;
	struct ifnet *ifp = in6m->in6m_ifp;
	int ignflags;

	/*
	 * At first, find a link local address on the outgoing interface
	 * to use as the source address of the MLD packet.
	 * We do not reject tentative addresses for MLD report to deal with
	 * the case where we first join a link-local address.
	 */
	ignflags = (IN6_IFF_NOTREADY|IN6_IFF_ANYCAST) & ~IN6_IFF_TENTATIVE;
	if ((ia = in6ifa_ifpforlinklocal(ifp, ignflags)) == NULL)
		return;
	if ((ia->ia6_flags & IN6_IFF_TENTATIVE))
		ia = NULL;

	/* Allocate two mbufs to store IPv6 header and MLD header */
	mldh = mld6_allocbuf(&mh, MLD_MINLEN, in6m, type);
	if (mldh == NULL)
		return;

	ip6 = mtod(mh, struct ip6_hdr *);
	/* ip6_hlim will be set by im6o.im6o_multicast_hlim */
	ip6->ip6_src = ia ? ia->ia_addr.sin6_addr : in6addr_any;
	ip6->ip6_dst = dst ? *dst : in6m->in6m_addr;

	mldh->mld_addr = in6m->in6m_addr;
	in6_clearscope(&mldh->mld_addr); /* XXX */
	if (IN6_IS_ADDR_MC_LINKLOCAL(&mldh->mld_addr)) {
		mldh->mld_addr.s6_addr16[1] = 0; /* XXX */
	}
	mldh->mld_cksum = in6_cksum(mh, IPPROTO_ICMPV6, sizeof(struct ip6_hdr), sizeof(struct mld_hdr));

	/* construct multicast option */
	bzero(&im6o, sizeof(im6o));
	im6o.im6o_multicast_ifp = ifp;
	im6o.im6o_multicast_hlim = 1;

	/*
	 * Request loopback of the report if we are acting as a multicast
	 * router, so that the process-level routing daemon can hear it.
	 */
	im6o.im6o_multicast_loop = (ip6_mrouter != NULL);

	/* increment output statictics */
	icmp6stat.icp6s_outhist[type]++;
	icmp6_ifstat_inc(ifp, ifs6_out_msg);
	switch (type) {
	case MLD_LISTENER_QUERY:
		icmp6_ifstat_inc(ifp, ifs6_out_mldquery);
		break;
	case MLD_LISTENER_REPORT:
		icmp6_ifstat_inc(ifp, ifs6_out_mldreport);
		break;
	case MLD_LISTENER_DONE:
		icmp6_ifstat_inc(ifp, ifs6_out_mlddone);
		break;
	}

	ip6_output(mh, &ip6_opts, NULL, ia ? 0 : IPV6_UNSPECSRC, &im6o,
			(struct socket*) NULL, NULL);
}

#ifdef MLDV2

/*
 * Parse source addresses from MLDv2 Group-and-Source-Specific Query message
 * and merge them in a recorded source list as specified in RFC3810 6.3 (3).
 * If the recorded source list cannot be kept in memory, return an error code.
 * If no pending source was recorded, return -1.
 * If some source was recorded as a reply for Group-and-Source-Specific Query,
 * return 0.
 */
static int
mld6_record_queried_source(in6m, mld, mldlen)
	struct in6_multi *in6m;
	struct mld_hdr *mld;
	u_int16_t mldlen;
{
	u_int16_t numsrc, i;
	int ref_count;
	struct sockaddr_in6 src;
	int recorded = 0;
	struct mldv2_hdr *mldh = (struct mldv2_hdr *) mld;

	mldlen -= qhdrlen; /* remaining source list */
	numsrc = ntohs(mldh->mld_numsrc);
	if (numsrc != mldlen / addrlen)
	    return EOPNOTSUPP; /* XXX */

	bzero(&src, sizeof(src));
	src.sin6_family = AF_INET6;
	src.sin6_len = sizeof(src);

	for (i = 0; i < numsrc && mldlen >= addrlen; i++, mldlen -= addrlen) {
		/* make a local copy for not overriding the packet content */
		src.sin6_addr = mldh->mld_src[i];
		if (in6_setscope(&src.sin6_addr, in6m->in6m_ifp, NULL)) {
			/* XXX: should be impossible, so continue anyway */
			continue;
		}
		if (match_msf6_per_if(in6m, &src.sin6_addr,
		    &in6m->in6m_addr) == 0)
			continue;

		ref_count = in6_merge_msf_source_addr(in6m->in6m_source->i6ms_rec,
		    &src, IMS_ADD_SOURCE);
		if (ref_count < 0) {
			if (in6m->in6m_source->i6ms_rec->numsrc)
				in6_free_msf_source_list(in6m->in6m_source->i6ms_rec->head);
			in6m->in6m_source->i6ms_rec->numsrc = 0;
			return ENOBUFS;
		}
		if (ref_count == 1)
			++in6m->in6m_source->i6ms_rec->numsrc;	/* new entry */

		recorded = 1;
	}

	return ((recorded == 0) ? -1 : 0);
}

/*
 * Send Current-State Report for General Query response.
 */
static void
mld6_send_all_current_state_report(ifp)
	struct ifnet *ifp;
{
	struct in6_multi *in6m;
	struct in6_multistep step;

	mldlog((LOG_DEBUG, "%s for %s\n", __FUNCTION__, if_name(ifp)));

	IN6_FIRST_MULTI(step, in6m);
	while (in6m != NULL) {
		if (in6m->in6m_ifp != ifp ||
		    !in6_is_mld_target(&in6m->in6m_addr))
			goto next_multi;

		if (mld_send_current_state_report(in6m) != 0)
			return;
next_multi:
		IN6_NEXT_MULTI(step, in6m);
	}
}

/*
 * Send Current-State Report for Group- and Group-and-Source-Sepcific Query
 * response.
 */
static int
mld6_send_current_state_report(in6m)
	struct in6_multi *in6m;
{
	struct mbuf *m = NULL;
	u_int16_t max_len;
	u_int16_t numsrc = 0, src_once, src_done = 0;
	u_int8_t type = 0;
	struct mld_hdr *mldh;

	mldlog((LOG_DEBUG, "%s for %s\n",
	    __FUNCTION__, ip6_sprintf(&in6m->in6m_addr)));

	if (!in6_is_mld_target(&in6m->in6m_addr) ||
		(in6m->in6m_ifp->if_flags & IFF_LOOPBACK) != 0)
	    return 0;

	/* MCLBYTES is the maximum length even if if_mtu is too big. */
	max_len = (in6m->in6m_ifp->if_mtu < MCLBYTES) ?
				in6m->in6m_ifp->if_mtu : MCLBYTES;

	if (in6m->in6m_source->i6ms_mode == MCAST_INCLUDE)
		type = MODE_IS_INCLUDE;
	else if (in6m->in6m_source->i6ms_mode == MCAST_EXCLUDE)
		type = MODE_IS_EXCLUDE;

	/*
	 * Prepare record for General, Group-Specific, and Group-and-Source-
	 * Specific Query.
	 */
	if (in6m->in6m_state == MLD_SG_QUERY_PENDING_MEMBER) {
		type = MODE_IS_INCLUDE; /* always */
		numsrc = in6m->in6m_source->i6ms_rec->numsrc;
	} else
		numsrc = in6m->in6m_source->i6ms_cur->numsrc;

	if (type == MODE_IS_INCLUDE && numsrc == 0)
		return 0; /* no need to send Current-State Report */

	/*
	 * If Report type is MODE_IS_EXCLUDE, a single Group Record is sent,
	 * containing as many source addresses as can fit, and the remaining
	 * source addresses are not reported.
	 */
	if (type == MODE_IS_EXCLUDE) {
		if (max_len < SOURCE_RECORD_LEN(numsrc)
		    + sizeof(struct ip6_hdr) + rhdrlen + ghdrlen)
			numsrc = (max_len - sizeof(struct ip6_hdr)
			    - rhdrlen - ghdrlen) / addrlen;
	}

	mldh = mld6_allocbuf(&m, rhdrlen, in6m, MLDV2_LISTENER_REPORT);
	if (mldh == NULL) {
		mldlog((LOG_DEBUG, "mld6_send_current_state_report: "
		    "error preparing new report header\n"));
		return ENOBUFS;
	}

	if (type == MODE_IS_EXCLUDE) {
		/*
		 * The number of sources of MODE_IS_EXCLUDE record is already
		 * adjusted to fit in one buffer.
		 */
		if (mld6_create_group_record(m, in6m, numsrc,
					   &src_done, type) != numsrc) {
			mldlog((LOG_DEBUG,
			    "mld6_send_current_state_report: "
			    "error of sending MODE_IS_EXCLUDE report?\n"));
			m_freem(m);
			return EOPNOTSUPP; /* XXX source address insert didn't
					    * finished. strange... */
		}
		if (m != NULL)
			mld6_sendbuf(m, in6m->in6m_ifp);
	} else {
		while (1) {
			/* XXX Some security implication? */
			src_once = mld6_create_group_record(m, in6m,
							   numsrc, &src_done,
							   type);

			if (numsrc <= src_done)
				 break;	/* finish insertion */
			/*
			 * Source address insert didn't finished, so, send this
			 * MLD report here and try to make separate message
			 * with remaining sources.
			 */
			mld6_sendbuf(m, in6m->in6m_ifp);
			mldh = mld6_allocbuf(&m, rhdrlen, in6m,
			    MLDV2_LISTENER_REPORT);
			if (mldh == NULL) {
				mldlog((LOG_DEBUG,
				    "mld6_send_current_state_report: "
				    "error preparing additional report "
				    "header.\n"));
				return ENOBUFS;
			}
		} /* while */
		if (m != NULL)
			mld6_sendbuf(m, in6m->in6m_ifp);
	}

	/*
	 * If pending query was Group-and-Source-Specific Query, pending
	 * (merged) source list is cleared.
	 */
	if (in6m->in6m_state == MLD_SG_QUERY_PENDING_MEMBER) {
		in6_free_msf_source_list(in6m->in6m_source->i6ms_rec->head);
		in6m->in6m_source->i6ms_rec->numsrc = 0;
	}
	in6m->in6m_state = MLD_OTHERLISTENER;
	in6m->in6m_timer = IN6M_TIMER_UNDEF;

	return 0;
}

/*
 * Send State-Change Report (Filter-Mode-Change Report and Source-List-Change
 * Report).
 * If report type is not specified (i.e., "0"), it indicates that requested
 * State-Change report may consist of Source-List-Change record. However, if
 * there is also a pending Filter-Mode-Change report for the group, Source-
 * List-Change report is not sent and kept as a scheduled report.
 *
 * timer_init - set this when IPMulticastListen() invoked
 */
void
mld6_send_state_change_report(in6m, type, timer_init)
	struct in6_multi *in6m;
	u_int8_t type;
	int timer_init;
{
	struct mbuf *m = NULL;
	u_int16_t max_len;
	u_int16_t numsrc = 0, src_once, src_done = 0;
	struct mld_hdr *mldh;

	mldlog((LOG_DEBUG, "%s for %s\n",
	    __FUNCTION__, ip6_sprintf(&in6m->in6m_addr)));

	if (!in6_is_mld_target(&in6m->in6m_addr) ||
		(in6m->in6m_ifp->if_flags & IFF_LOOPBACK) != 0)
		return;

	/*
	 * If there is pending Filter-Mode-Change report, Source-List-Change
	 * report will be merged in an another message and scheduled to be
	 * sent after Filter-Mode-Change report is sent.
	 */
	if (in6m->in6m_source->i6ms_toex != NULL) {
		/* Initial TO_EX request must specify "type". */
		if (type == 0) {
			if (timer_init &&
			    ((in6m->in6m_source->i6ms_alw != NULL &&
			    in6m->in6m_source->i6ms_alw->numsrc > 0) ||
			    (in6m->in6m_source->i6ms_blk != NULL &&
			    in6m->in6m_source->i6ms_blk->numsrc > 0)))
				return; /* scheduled later */
			type = CHANGE_TO_EXCLUDE_MODE;
		}
	}
	if (in6m->in6m_source->i6ms_toin != NULL) {
		/* Initial TO_IN request must specify "type". */
		if (type == 0) {
			if (timer_init &&
			    ((in6m->in6m_source->i6ms_alw != NULL &&
			    in6m->in6m_source->i6ms_alw->numsrc > 0) ||
			    (in6m->in6m_source->i6ms_blk != NULL &&
			    in6m->in6m_source->i6ms_blk->numsrc > 0)))
				return; /* scheduled later */

			type = CHANGE_TO_INCLUDE_MODE;
		}
	}

	if (timer_init)
		in6m->in6m_source->i6ms_robvar = in6m->in6m_rti->rt6i_qrv;

	/* MCLBYTES is the maximum length even if if_mtu is too big. */
	max_len = (in6m->in6m_ifp->if_mtu < MCLBYTES) ?
				in6m->in6m_ifp->if_mtu : MCLBYTES;

	/*
	 * If Report type is CHANGE_TO_EXCLUDE_MODE, a single Group Record
	 * is sent, containing as many source addresses as can fit, and the
	 * remaining source addresses are not reported.
	 * Note that numsrc may or may not be 0.
	 */
	if (type == CHANGE_TO_EXCLUDE_MODE) {
		if (in6m->in6m_source->i6ms_toex != NULL)
			numsrc = in6m->in6m_source->i6ms_toex->numsrc;
		if (max_len < SOURCE_RECORD_LEN(numsrc)
			+ sizeof(struct ip6_hdr) + rhdrlen + ghdrlen)
			/* toex's numsrc should be fit in a single message. */
			numsrc = (max_len - sizeof(struct ip6_hdr)
				- rhdrlen - ghdrlen) / addrlen;
	} else if (type == CHANGE_TO_INCLUDE_MODE) {
		if (in6m->in6m_source->i6ms_toin != NULL)
			numsrc = in6m->in6m_source->i6ms_toin->numsrc;
	}

	mldh = mld6_allocbuf(&m, rhdrlen, in6m, MLDV2_LISTENER_REPORT);
	if (mldh == NULL) {
		mldlog((LOG_DEBUG,
		    "mld6_send_state_change_report: "
		    "error preparing new report header.\n"));
		return; /* robvar is not reduced */
	}

	/* creates records based on type and finish this function */
	if (type == CHANGE_TO_EXCLUDE_MODE) {
		/*
		 * The number of sources of CHANGE_TO_EXCLUDE_MODE
		 * record is already adjusted to fit in one buffer.
		 */
		if (mld6_create_group_record(m, in6m, numsrc,
					    &src_done, type) != numsrc) {
			mldlog((LOG_DEBUG, "mld6_send_state_change_report: "
				"error of sending "
				"CHANGE_TO_EXCLUDE_MODE report?\n"));
			m_freem(m);
			return;
			/* XXX source address insert didn't finished.
			* strange... robvar is not reduced */
		}

		if (m != NULL) {
			mld6_sendbuf(m, in6m->in6m_ifp);
			m = NULL;
		}

		in6m->in6m_source->i6ms_robvar--;
		if (in6m->in6m_source->i6ms_robvar != 0)
			return;

		/* robustness variable is 0 -> cease the report timer */
		if (in6m->in6m_source->i6ms_toex != NULL) {
			/* For TO_EX list, it MUST be deleted after
			 * retransmission is done. This is because
			 * mld_state_change_timeo() doesn't know if the
			 * pending TO_EX report exists or not. */
			 in6_free_msf_source_list(in6m->in6m_source->i6ms_toex->head);
			 FREE(in6m->in6m_source->i6ms_toex->head, M_MSFILTER);
			 FREE(in6m->in6m_source->i6ms_toex, M_MSFILTER);
			 in6m->in6m_source->i6ms_toex = NULL;
		}

		/* Prepare scheduled Source-List-Change Report if necessary */
		 if ((in6m->in6m_source->i6ms_alw != NULL &&
		     in6m->in6m_source->i6ms_alw->numsrc > 0) ||
		     (in6m->in6m_source->i6ms_blk != NULL &&
		     in6m->in6m_source->i6ms_blk->numsrc > 0)) {
			in6m->in6m_source->i6ms_robvar = in6m->in6m_rti->rt6i_qrv;
			return;
		}
		return;
	}

	if (type == CHANGE_TO_INCLUDE_MODE) {
		while (1) {
			/* XXX Some security implication? */
			src_once = mld6_create_group_record(m, in6m, numsrc,
					&src_done, type);
			if (numsrc <= src_done)
				break;	/* finish insertion */

			mld6_sendbuf(m, in6m->in6m_ifp);
			m = NULL;
			mldh = mld6_allocbuf(&m, rhdrlen, in6m,
			    MLDV2_LISTENER_REPORT);
			if (mldh == NULL) {
				mldlog((LOG_DEBUG,
					"mld6_send_state_change_report: "
					"error preparing additional report "
					"header.\n"));
				return;
			}
		}
		if (m != NULL) {
			mld6_sendbuf(m, in6m->in6m_ifp);
			m = NULL;
		}

		in6m->in6m_source->i6ms_robvar--;
		if (in6m->in6m_source->i6ms_robvar != 0)
			return;

		/* robustness variable is 0 -> cease the report timer */
		if (in6m->in6m_source->i6ms_toin != NULL) {
			/* For TO_IN list, it MUST be deleted
			 * after retransmission is done. This is
			 * because mld_state_change_timeo() doesn't know
			 * if the pending TO_IN report exists or not
			 */
			in6_free_msf_source_list(in6m->in6m_source->i6ms_toin->head);
			FREE(in6m->in6m_source->i6ms_toin->head, M_MSFILTER);
			FREE(in6m->in6m_source->i6ms_toin, M_MSFILTER);
			in6m->in6m_source->i6ms_toin = NULL;
		}

		/* Prepare scheduled Source-List-Change Report if necessary */
		if ((in6m->in6m_source->i6ms_alw != NULL &&
		    in6m->in6m_source->i6ms_alw->numsrc > 0) ||
		     (in6m->in6m_source->i6ms_blk != NULL &&
		     in6m->in6m_source->i6ms_blk->numsrc > 0)) {
			in6m->in6m_source->i6ms_robvar = in6m->in6m_rti->rt6i_qrv;
			return;
		}
		return;
	}

	/*
	 * ALLOW_NEW_SOURCES and/or BLOCK_OLD_SOURCES
	 * You have to scan allow-list and block-list sequencially,
	 * since both list might have a source address to be announced.
	 */
	if ((in6m->in6m_source->i6ms_alw != NULL) &&
	    (in6m->in6m_source->i6ms_alw->numsrc != 0)) {
		type = ALLOW_NEW_SOURCES;
		numsrc = in6m->in6m_source->i6ms_alw->numsrc;
		while (1) {
			src_once = mld6_create_group_record(m, in6m, numsrc,
			    &src_done, type);
			mld6_sendbuf(m, in6m->in6m_ifp);
			if (numsrc <= src_done)
				break;

			mldlog((LOG_DEBUG,
				"mld6_send_current_state_report: "
				"re-allocbuf(allow)\n"));
			mldh = mld6_allocbuf(&m, rhdrlen, in6m,
				    MLDV2_LISTENER_REPORT);
			if (mldh == NULL) {
				mldlog((LOG_DEBUG,
					"mld6_send_state_change_report: "
					"error preparing additional ALLOW"
					"header.\n"));
				return;
			}
		}
	}
	if ((in6m->in6m_source->i6ms_blk != NULL) &&
	    (in6m->in6m_source->i6ms_blk->numsrc != 0)) {
		type = BLOCK_OLD_SOURCES;
		numsrc = in6m->in6m_source->i6ms_blk->numsrc;
		while (1) {
			src_once = mld6_create_group_record(m, in6m, numsrc,
			    &src_done, type);
			mld6_sendbuf(m, in6m->in6m_ifp);
			if (numsrc <= src_done)
				break;

			mldlog((LOG_DEBUG,
				"mld6_send_current_state_report: "
				"re-allocbuf(block)\n"));
			mldh = mld6_allocbuf(&m, rhdrlen, in6m,
			    MLDV2_LISTENER_REPORT);
			if (mldh == NULL) {
				mldlog((LOG_DEBUG,
					"mld6_send_state_change_report: "
					"error preparing additional BLOCK"
					"header.\n"));
				return;
			}
		}
	}

	in6m->in6m_source->i6ms_robvar--;
	if (in6m->in6m_source->i6ms_robvar != 0)
		return;

	/* robustness variable is 0 -> cease the report timer */
	if ((in6m->in6m_source->i6ms_alw != NULL) &&
	    (in6m->in6m_source->i6ms_alw->numsrc != 0)) {
		in6_free_msf_source_list(in6m->in6m_source->i6ms_alw->head);
		in6m->in6m_source->i6ms_alw->numsrc = 0;
	}
	if ((in6m->in6m_source->i6ms_blk != NULL) &&
    	    (in6m->in6m_source->i6ms_blk->numsrc != 0)) {
	    	in6_free_msf_source_list(in6m->in6m_source->i6ms_blk->head);
		in6m->in6m_source->i6ms_blk->numsrc = 0;
	}
	return;
}

static int
mld6_create_group_record(mh, in6m, numsrc, done, type)
	struct mbuf *mh;
	struct in6_multi *in6m;
	u_int16_t numsrc, done;
	u_int8_t type;
{
	/* assumes IPv6 header and MLD data are separeted into different mbuf */
	struct mbuf *md = mh->m_next;
	struct ip6_hdr *ip6;
	struct mld_report_hdr *mld_rhdr;
	struct mld_group_record_hdr *mld_ghdr;
	struct in6_addr_source *ias;
	struct in6_addr_slist *iasl = NULL;
	u_int16_t i, total;
	int mfreelen;
	u_int16_t iplen;

	ip6 = mtod(mh, struct ip6_hdr *);
	iplen = ntohs(ip6->ip6_plen);
	mld_rhdr = mtod(md, struct mld_report_hdr *);
	++mld_rhdr->mld_grpnum;

	mld_ghdr = (struct mld_group_record_hdr *) ((char *)(mld_rhdr + 1));
	mld_ghdr->record_type = type;
	mld_ghdr->auxlen = 0;
	mld_ghdr->numsrc = 0;
	bcopy(&in6m->in6m_addr, &mld_ghdr->group, sizeof(mld_ghdr->group));
	in6_clearscope(&mld_ghdr->group); /* XXX */
	md->m_len += ghdrlen;
	iplen += ghdrlen;
	mh->m_pkthdr.len += ghdrlen;
	mfreelen = MCLBYTES;

	/* get the head of the appropriate source list */
	switch (type) {
	case ALLOW_NEW_SOURCES:
		iasl = in6m->in6m_source->i6ms_alw;
		break;
	case BLOCK_OLD_SOURCES:
		iasl = in6m->in6m_source->i6ms_blk;
		break;
	case CHANGE_TO_INCLUDE_MODE:
		iasl = in6m->in6m_source->i6ms_toin;
		break;
	case CHANGE_TO_EXCLUDE_MODE:
		iasl = in6m->in6m_source->i6ms_toex;
		break;
	default:
		if (in6m->in6m_state == MLD_SG_QUERY_PENDING_MEMBER)
			iasl = in6m->in6m_source->i6ms_rec;
		else
			iasl = in6m->in6m_source->i6ms_cur;
		break;
	}

	total = 0;
	i = 0;
	if (iasl != NULL) {
		for (ias = LIST_FIRST(iasl->head); total < *done;
				total++, ias = LIST_NEXT(ias, i6as_list))
			; /* adjust a source pointer. */
		/* Insert source address to mbuf */
		for (; i < numsrc && ias != NULL && mfreelen > addrlen;
				i++, total++, mfreelen -= addrlen,
				ias = LIST_NEXT(ias, i6as_list)) {
			bcopy(&ias->i6as_addr.sin6_addr,
			      &mld_ghdr->src[i], sizeof(mld_ghdr->src[i]));
			in6_clearscope(&mld_ghdr->src[i]); /* XXX */
		}
	}

	*done = total;

	mld_ghdr->numsrc = i;
	md->m_len += SOURCE_RECORD_LEN(i);
	iplen += SOURCE_RECORD_LEN(i);
	ip6->ip6_plen = htons(iplen);
	mh->m_pkthdr.len += SOURCE_RECORD_LEN(i);

	return i;
}

/*
 * Cancel all MLDv2 pending response and retransmission timers on an
 * interface.
 */
static void
mld6_cancel_pending_response(ifp, rti)
	struct ifnet *ifp;
	struct router6_info *rti;
{
	struct in6_multi *in6m;
	struct in6_multistep step;

	rti->rt6i_timer2 = 0;
	IN6_FIRST_MULTI(step, in6m);
	while (in6m != NULL) {
		if (in6m->in6m_ifp != ifp)
			goto next_multi;
		if (!in6_is_mld_target(&in6m->in6m_addr))
			goto next_multi;
		if (in6m->in6m_source == NULL)
			goto next_multi;

		in6m->in6m_source->i6ms_robvar = 0;
		i6ms->i6ms_timer = 0;
		in6_free_msf_source_list(in6m->in6m_source->i6ms_rec->head);
		in6m->in6m_source->i6ms_rec->numsrc = 0;
		if (in6m->in6m_source->i6ms_alw != NULL) {
			in6_free_msf_source_list(in6m->in6m_source->i6ms_alw->head);
			in6m->in6m_source->i6ms_alw->numsrc = 0;
		}
		if (in6m->in6m_source->i6ms_blk != NULL) {
			in6_free_msf_source_list(in6m->in6m_source->i6ms_blk->head);
			in6m->in6m_source->i6ms_blk->numsrc = 0;
		}
		if (in6m->in6m_source->i6ms_toin != NULL) {
			in6_free_msf_source_list(in6m->in6m_source->i6ms_toin->head);
			/* For TO_IN list, it MUST be deleted. */
			FREE(in6m->in6m_source->i6ms_toin->head, M_MSFILTER);
			FREE(in6m->in6m_source->i6ms_toin, M_MSFILTER);
			in6m->in6m_source->i6ms_toin = NULL;
		}
		if (in6m->in6m_source->i6ms_toex != NULL) {
			in6_free_msf_source_list(in6m->in6m_source->i6ms_toex->head);
			/* For TO_EX list, it MUST be deleted. */
			FREE(in6m->in6m_source->i6ms_toex->head, M_MSFILTER);
			FREE(in6m->in6m_source->i6ms_toex, M_MSFILTER);
			in6m->in6m_source->i6ms_toex = NULL;
		}

next_multi:
		IN6_NEXT_MULTI(step, in6m);
	}
}

#endif
