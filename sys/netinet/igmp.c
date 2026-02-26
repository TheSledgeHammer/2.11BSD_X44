/*	$NetBSD: igmp.c,v 1.36 2003/08/22 21:53:02 itojun Exp $	*/

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
 * Internet Group Management Protocol (IGMP) routines.
 *
 * Written by Steve Deering, Stanford, May 1988.
 * Modified by Rosen Sharma, Stanford, Aug 1994.
 * Modified by Bill Fenner, Xerox PARC, Feb 1995.
 *
 * MULTICAST Revision: 1.3
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: igmp.c,v 1.36 2003/08/22 21:53:02 itojun Exp $");

#include "opt_mrouting.h"

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>

#include <machine/stdarg.h>

#define IP_MULTICASTOPTS	0

struct igmpstat igmpstat;
int igmp_timers_are_running;
static LIST_HEAD(, router_info) rti_head = LIST_HEAD_INITIALIZER(rti_head);

int interface_timers_are_running;
int state_change_timers_are_running;
#ifdef IGMPV3
int igmpsendwithra = 1;		/* send packets with no Router Alert option */
#else
int igmpsendwithra = 0;
#endif
int igmpdropwithnora = 0;	/* accept packets with no Router Alert option */
int igmpmaxsrcfilter = IP_MAX_SOURCE_FILTER;
int igmpsomaxsrc = SO_MAX_SOURCE_FILTER;

/*
 * igmp_version:
 *	0: igmpv3 with compat-mode
 *	1: igmpv1 only
 *	2: igmpv2 only
 *	3: igmpv3 without compat-mode
 */
int igmp_version = 0;
static struct mbuf *router_alert;
#ifdef IGMPV3
static int qhdrlen = IGMP_v3_QUERY_MINLEN;
static int rhdrlen = IGMP_MINLEN;
static int ghdrlen = IGMP_MINLEN;
static int addrlen = sizeof(struct in_addr);
#endif

#define	SOURCE_RECORD_LEN(numsrc)	(numsrc * (sizeof(u_int32_t)))

#define	GET_REPORT_SOURCE_HEAD(inm, type, iasl) {				\
	if ((type) == ALLOW_NEW_SOURCES)							\
		(iasl) = (inm)->inm_source->ims_alw;					\
	else if ((type) == BLOCK_OLD_SOURCES)						\
		(iasl) = (inm)->inm_source->ims_blk;					\
	else if ((type) == CHANGE_TO_INCLUDE_MODE)					\
		(iasl) = (inm)->inm_source->ims_toin;					\
	else if ((type) == CHANGE_TO_EXCLUDE_MODE)					\
		(iasl) = (inm)->inm_source->ims_toex;					\
	else {														\
		if ((inm)->inm_state == IGMP_SG_QUERY_PENDING_MEMBER)	\
			(iasl) = (inm)->inm_source->ims_rec;				\
		else													\
			(iasl) = (inm)->inm_source->ims_cur;				\
	}															\
}

#define	SET_REPORTHDR(m, numsrc) do {							\
	MGETHDR((m), M_DONTWAIT, MT_HEADER);						\
	if ((m) != NULL &&											\
		MHLEN - max_linkhdr < sizeof(struct ip)					\
				      + rhdrlen + ghdrlen						\
				      + SOURCE_RECORD_LEN((numsrc))) {			\
		MCLGET((m), M_DONTWAIT);								\
		if (((m)->m_flags & M_EXT) == 0) {						\
			m_freem((m));										\
			error = ENOBUFS;									\
			break;												\
		}														\
	}															\
	if ((m) == NULL) {											\
		error = ENOBUFS;										\
		break;													\
	}															\
	(m)->m_data += max_linkhdr;									\
	ip = mtod((m), struct ip *);								\
	buflen = sizeof(struct ip);									\
	ip->ip_len = sizeof(struct ip) + rhdrlen;					\
	ip->ip_tos = 0xc0;											\
	ip->ip_off = 0;												\
	ip->ip_p = IPPROTO_IGMP;									\
	ip->ip_src = zeroin_addr;									\
	ip->ip_dst.s_addr = INADDR_NEW_ALLRTRS_GROUP;				\
	igmp_rhdr = (struct igmp_report_hdr *)((char *)ip + buflen);\
	igmp_rhdr->igmp_type = IGMP_v3_HOST_MEMBERSHIP_REPORT;		\
	igmp_rhdr->igmp_reserved1 = 0;								\
	igmp_rhdr->igmp_reserved2 = 0;								\
	igmp_rhdr->igmp_grpnum = 0;									\
	buflen += rhdrlen;											\
	(m)->m_len = sizeof(struct ip) + rhdrlen;					\
	(m)->m_pkthdr.len = sizeof(struct ip) + rhdrlen;			\
	(m)->m_pkthdr.rcvif = (struct ifnet *)0;					\
} while (0)

void igmp_sendpkt(struct in_multi *, int);
static int rti_fill(struct in_multi *);
static struct router_info *rti_find(struct ifnet *);
static void rti_delete(struct ifnet *);

void
igmp_init(void)
{
	struct ipoption *ra;

	igmp_timers_are_running = 0;

	interface_timers_are_running = 0; /* used only by IGMPv3 */
	state_change_timers_are_running = 0; /* used only by IGMPv3 */

	/*
	 * Prepare Router Alert option to use in outgoing packets.
	 */
	MGET(router_alert, M_DONTWAIT, MT_DATA);
	ra = mtod(router_alert, struct ipoption *);
	ra->ipopt_dst.s_addr = 0;
	ra->ipopt_list[0] = IPOPT_RA;	/* Router Alert option */
	ra->ipopt_list[1] = 0x04;	/* 4 bytes long */
	ra->ipopt_list[2] = 0x00;
	ra->ipopt_list[3] = 0x00;
	router_alert->m_len = sizeof(ra->ipopt_dst) + ra->ipopt_list[1];
}

struct router_info *
rti_init(ifp)
	struct ifnet *ifp;
{
	struct router_info *rti;

	rti = (struct router_info *)malloc(sizeof(struct router_info), M_MRTABLE, M_NOWAIT);
	if (rti == NULL)
		return NULL;

	rti->rti_ifp = ifp;
#ifndef IGMPV3
	rti->rti_type = IGMP_v2_ROUTER;
#else
	rti->rti_timer1 = 0;
	rti->rti_timer2 = 0;
	rti->rti_timer3 = 0;
	rti->rti_qrv = IGMP_DEF_RV;
	rti->rti_qqi = IGMP_DEF_QI;
	rti->rti_qri = IGMP_DEF_QRI / IGMP_TIMER_SCALE;
	switch (igmp_version) {
	case 0:
		rti->rti_type = IGMP_v3_ROUTER;
		break;
	case 1:
		rti->rti_type = IGMP_v1_ROUTER;
		break;
	case 2:
		rti->rti_type = IGMP_v2_ROUTER;
		break;
	case 3:
		rti->rti_type = IGMP_v3_ROUTER;
		break;
	default:
		/* impossible */
		break;
	}
#endif
	LIST_INSERT_HEAD(&rti_head, rti, rti_link);
	return (rti);
}

static int
rti_fill(inm)
	struct in_multi *inm;
{
	struct router_info *rti;

	LIST_FOREACH(rti, &rti_head, rti_link) {
		if (rti->rti_ifp == inm->inm_ifp) {
			inm->inm_rti = rti;
			if (rti->rti_type == IGMP_v1_ROUTER) {
				return (IGMP_v1_HOST_MEMBERSHIP_REPORT);
			} else {
				return (IGMP_v2_HOST_MEMBERSHIP_REPORT);
			}
		}
	}

	rti = rti_init(inm->inm_ifp);
	if (rti == NULL) {
		return 0;
	}
	inm->inm_rti = rti;
	return (IGMP_v2_HOST_MEMBERSHIP_REPORT);
}

static struct router_info *
rti_find(ifp)
	struct ifnet *ifp;
{
	struct router_info *rti;

	LIST_FOREACH(rti, &rti_head, rti_link) {
		if (rti->rti_ifp == ifp)
			return (rti);
	}

	rti = rti_init(ifp);
	if (rti == NULL) {
		return NULL;
	}
	return (rti);
}

static void
rti_delete(ifp)
	struct ifnet *ifp;
{
	struct router_info *rti;

	LIST_FOREACH(rti, &rti_head, rti_link) {
		if (rti->rti_ifp == ifp) {
			LIST_REMOVE(rti, rti_link);
			free(rti, M_MRTABLE);
			return;
		}
	}
}

void
igmp_input(struct mbuf *m, ...)
{
	int proto;
	int iphlen;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct ip *ip = mtod(m, struct ip *);
	struct igmp *igmp;
	u_int minlen;
	struct in_multi *inm;
	struct in_multistep step;
	struct router_info *rti;
	struct in_ifaddr *ia;
	u_int timer;
	va_list ap;
	u_int16_t ip_len;

	va_start(ap, m);
	iphlen = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

	++igmpstat.igps_rcv_total;

	/*
	 * Validate lengths
	 */
	minlen = iphlen + IGMP_MINLEN;
	ip_len = ntohs(ip->ip_len);
	if (ip_len < minlen) {
		++igmpstat.igps_rcv_tooshort;
		m_freem(m);
		return;
	}
	if (((m->m_flags & M_EXT) && (ip->ip_src.s_addr & IN_CLASSA_NET) == 0)
	    || m->m_len < minlen) {
		if ((m = m_pullup(m, minlen)) == 0) {
			++igmpstat.igps_rcv_tooshort;
			return;
		}
		ip = mtod(m, struct ip *);
	}

	/*
	 * Validate checksum
	 */
	m->m_data += iphlen;
	m->m_len -= iphlen;
	igmp = mtod(m, struct igmp *);
	/* No need to assert alignment here. */
	if (in_cksum(m, ip_len - iphlen)) {
		++igmpstat.igps_rcv_badsum;
		m_freem(m);
		return;
	}
	m->m_data -= iphlen;
	m->m_len += iphlen;

	switch (igmp->igmp_type) {

	case IGMP_HOST_MEMBERSHIP_QUERY:
		++igmpstat.igps_rcv_queries;

		if (ifp->if_flags & IFF_LOOPBACK)
			break;

		if (igmp->igmp_code == 0) {
			rti = rti_find(ifp);
			if (rti == NULL)
				break;
			rti->rti_type = IGMP_v1_ROUTER;
			rti->rti_age = 0;

			if (ip->ip_dst.s_addr != INADDR_ALLHOSTS_GROUP) {
				++igmpstat.igps_rcv_badqueries;
				m_freem(m);
				return;
			}

			/*
			 * Start the timers in all of our membership records
			 * for the interface on which the query arrived,
			 * except those that are already running and those
			 * that belong to a "local" group (224.0.0.X).
			 */
			IN_FIRST_MULTI(step, inm);
			while (inm != NULL) {
				if (inm->inm_ifp == ifp &&
				    inm->inm_timer == 0 &&
				    !IN_LOCAL_GROUP(inm->inm_addr.s_addr)) {
					inm->inm_state = IGMP_DELAYING_MEMBER;
					inm->inm_timer = IGMP_RANDOM_DELAY(
					    IGMP_MAX_HOST_REPORT_DELAY * PR_FASTHZ);
					igmp_timers_are_running = 1;
				}
				IN_NEXT_MULTI(step, inm);
			}
		} else {
			if (!IN_MULTICAST(ip->ip_dst.s_addr)) {
				++igmpstat.igps_rcv_badqueries;
				m_freem(m);
				return;
			}

			timer = igmp->igmp_code * PR_FASTHZ / IGMP_TIMER_SCALE;
			if (timer == 0)
				timer =1;

			/*
			 * Start the timers in all of our membership records
			 * for the interface on which the query arrived,
			 * except those that are already running and those
			 * that belong to a "local" group (224.0.0.X).  For
			 * timers already running, check if they need to be
			 * reset.
			 */
			IN_FIRST_MULTI(step, inm);
			while (inm != NULL) {
				if (inm->inm_ifp == ifp &&
				    !IN_LOCAL_GROUP(inm->inm_addr.s_addr) &&
				    (ip->ip_dst.s_addr == INADDR_ALLHOSTS_GROUP ||
				     in_hosteq(ip->ip_dst, inm->inm_addr))) {
					switch (inm->inm_state) {
					case IGMP_DELAYING_MEMBER:
						if (inm->inm_timer <= timer)
							break;
						/* FALLTHROUGH */
					case IGMP_IDLE_MEMBER:
					case IGMP_LAZY_MEMBER:
					case IGMP_AWAKENING_MEMBER:
						inm->inm_state =
						    IGMP_DELAYING_MEMBER;
						inm->inm_timer =
						    IGMP_RANDOM_DELAY(timer);
						igmp_timers_are_running = 1;
						break;
					case IGMP_SLEEPING_MEMBER:
						inm->inm_state =
						    IGMP_AWAKENING_MEMBER;
						break;
					}
				}
				IN_NEXT_MULTI(step, inm);
			}
		}

		break;

	case IGMP_v1_HOST_MEMBERSHIP_REPORT:
		++igmpstat.igps_rcv_reports;

		if (ifp->if_flags & IFF_LOOPBACK)
			break;

		if (!IN_MULTICAST(igmp->igmp_group.s_addr) ||
		    !in_hosteq(igmp->igmp_group, ip->ip_dst)) {
			++igmpstat.igps_rcv_badreports;
			m_freem(m);
			return;
		}

		/*
		 * KLUDGE: if the IP source address of the report has an
		 * unspecified (i.e., zero) subnet number, as is allowed for
		 * a booting host, replace it with the correct subnet number
		 * so that a process-level multicast routing daemon can
		 * determine which subnet it arrived from.  This is necessary
		 * to compensate for the lack of any way for a process to
		 * determine the arrival interface of an incoming packet.
		 */
		if ((ip->ip_src.s_addr & IN_CLASSA_NET) == 0) {
			IFP_TO_IA(ifp, ia);		/* XXX */
			if (ia)
				ip->ip_src.s_addr = ia->ia_subnet;
		}

		/*
		 * If we belong to the group being reported, stop
		 * our timer for that group.
		 */
		IN_LOOKUP_MULTI(igmp->igmp_group, ifp, inm);
		if (inm != NULL) {
			inm->inm_timer = 0;
			++igmpstat.igps_rcv_ourreports;

			switch (inm->inm_state) {
			case IGMP_IDLE_MEMBER:
			case IGMP_LAZY_MEMBER:
			case IGMP_AWAKENING_MEMBER:
			case IGMP_SLEEPING_MEMBER:
				inm->inm_state = IGMP_SLEEPING_MEMBER;
				break;
			case IGMP_DELAYING_MEMBER:
				if (inm->inm_rti->rti_type == IGMP_v1_ROUTER)
					inm->inm_state = IGMP_LAZY_MEMBER;
				else
					inm->inm_state = IGMP_SLEEPING_MEMBER;
				break;
			}
		}

		break;

	case IGMP_v2_HOST_MEMBERSHIP_REPORT:
#ifdef MROUTING
		/*
		 * Make sure we don't hear our own membership report.  Fast
		 * leave requires knowing that we are the only member of a
		 * group.
		 */
		IFP_TO_IA(ifp, ia);			/* XXX */
		if (ia && in_hosteq(ip->ip_src, ia->ia_addr.sin_addr))
			break;
#endif

		++igmpstat.igps_rcv_reports;

		if (ifp->if_flags & IFF_LOOPBACK)
			break;

		if (!IN_MULTICAST(igmp->igmp_group.s_addr) ||
		    !in_hosteq(igmp->igmp_group, ip->ip_dst)) {
			++igmpstat.igps_rcv_badreports;
			m_freem(m);
			return;
		}

		/*
		 * KLUDGE: if the IP source address of the report has an
		 * unspecified (i.e., zero) subnet number, as is allowed for
		 * a booting host, replace it with the correct subnet number
		 * so that a process-level multicast routing daemon can
		 * determine which subnet it arrived from.  This is necessary
		 * to compensate for the lack of any way for a process to
		 * determine the arrival interface of an incoming packet.
		 */
		if ((ip->ip_src.s_addr & IN_CLASSA_NET) == 0) {
#ifndef MROUTING
			IFP_TO_IA(ifp, ia);		/* XXX */
#endif
			if (ia)
				ip->ip_src.s_addr = ia->ia_subnet;
		}

		/*
		 * If we belong to the group being reported, stop
		 * our timer for that group.
		 */
		IN_LOOKUP_MULTI(igmp->igmp_group, ifp, inm);
		if (inm != NULL) {
			inm->inm_timer = 0;
			++igmpstat.igps_rcv_ourreports;

			switch (inm->inm_state) {
			case IGMP_DELAYING_MEMBER:
			case IGMP_IDLE_MEMBER:
			case IGMP_AWAKENING_MEMBER:
				inm->inm_state = IGMP_LAZY_MEMBER;
				break;
			case IGMP_LAZY_MEMBER:
			case IGMP_SLEEPING_MEMBER:
				break;
			}
		}

		break;

	}

	/*
	 * Pass all valid IGMP packets up to any process(es) listening
	 * on a raw IGMP socket.
	 */
	rip_input(m, iphlen, proto);
	return;
}

int
igmp_joingroup(inm)
	struct in_multi *inm;
{
	int report_type;
	int s = splsoftnet();

	inm->inm_state = IGMP_IDLE_MEMBER;

	if (!IN_LOCAL_GROUP(inm->inm_addr.s_addr) &&
	    (inm->inm_ifp->if_flags & IFF_LOOPBACK) == 0) {
		report_type = rti_fill(inm);
		if (report_type == 0)
			return ENOMEM;
		igmp_sendpkt(inm, report_type);
		inm->inm_state = IGMP_DELAYING_MEMBER;
		inm->inm_timer = IGMP_RANDOM_DELAY(
		    IGMP_MAX_HOST_REPORT_DELAY * PR_FASTHZ);
		igmp_timers_are_running = 1;
	} else
		inm->inm_timer = 0;
	splx(s);
	return 0;
}

void
igmp_leavegroup(inm)
	struct in_multi *inm;
{

	switch (inm->inm_state) {
	case IGMP_DELAYING_MEMBER:
	case IGMP_IDLE_MEMBER:
		if (!IN_LOCAL_GROUP(inm->inm_addr.s_addr) &&
		    (inm->inm_ifp->if_flags & IFF_LOOPBACK) == 0)
			if (inm->inm_rti->rti_type != IGMP_v1_ROUTER)
				igmp_sendpkt(inm, IGMP_HOST_LEAVE_MESSAGE);
		break;
	case IGMP_LAZY_MEMBER:
	case IGMP_AWAKENING_MEMBER:
	case IGMP_SLEEPING_MEMBER:
		break;
	}
}

void
igmp_fasttimo(void)
{
	struct in_multi *inm;
	struct in_multistep step;
	int s;

	/*
	 * Quick check to see if any work needs to be done, in order
	 * to minimize the overhead of fasttimo processing.
	 */
	if (!igmp_timers_are_running)
		return;

	s = splsoftnet();
	igmp_timers_are_running = 0;
	IN_FIRST_MULTI(step, inm);
	while (inm != NULL) {
		if (inm->inm_timer == 0) {
			/* do nothing */
		} else if (--inm->inm_timer == 0) {
			if (inm->inm_state == IGMP_DELAYING_MEMBER) {
				if (inm->inm_rti->rti_type == IGMP_v1_ROUTER)
					igmp_sendpkt(inm,
					    IGMP_v1_HOST_MEMBERSHIP_REPORT);
				else
					igmp_sendpkt(inm,
					    IGMP_v2_HOST_MEMBERSHIP_REPORT);
				inm->inm_state = IGMP_IDLE_MEMBER;
			}
		} else {
			igmp_timers_are_running = 1;
		}
		IN_NEXT_MULTI(step, inm);
	}
	splx(s);
}

void
igmp_slowtimo(void)
{
	struct router_info *rti;
	int s;

	s = splsoftnet();
	LIST_FOREACH(rti, &rti_head, rti_link) {
		if (rti->rti_type == IGMP_v1_ROUTER &&
		    ++rti->rti_age >= IGMP_AGE_THRESHOLD) {
			rti->rti_type = IGMP_v2_ROUTER;
		}
	}
	splx(s);
}

void
igmp_sendpkt(inm, type)
	struct in_multi *inm;
	int type;
{
	struct mbuf *m;
	struct igmp *igmp;
	struct ip *ip;
	struct ip_moptions imo;
#ifdef MROUTING
	extern struct socket *ip_mrouter;
#endif /* MROUTING */

	MGETHDR(m, M_DONTWAIT, MT_HEADER);
	if (m == NULL)
		return;
	/*
	 * Assume max_linkhdr + sizeof(struct ip) + IGMP_MINLEN
	 * is smaller than mbuf size returned by MGETHDR.
	 */
	m->m_data += max_linkhdr;
	m->m_len = sizeof(struct ip) + IGMP_MINLEN;
	m->m_pkthdr.len = sizeof(struct ip) + IGMP_MINLEN;

	ip = mtod(m, struct ip *);
	ip->ip_tos = 0;
	ip->ip_len = htons(sizeof(struct ip) + IGMP_MINLEN);
	ip->ip_off = htons(0);
	ip->ip_p = IPPROTO_IGMP;
	ip->ip_src = zeroin_addr;
	ip->ip_dst = inm->inm_addr;

	m->m_data += sizeof(struct ip);
	m->m_len -= sizeof(struct ip);
	igmp = mtod(m, struct igmp *);
	igmp->igmp_type = type;
	igmp->igmp_code = 0;
	igmp->igmp_group = inm->inm_addr;
	igmp->igmp_cksum = 0;
	igmp->igmp_cksum = in_cksum(m, IGMP_MINLEN);
	m->m_data -= sizeof(struct ip);
	m->m_len += sizeof(struct ip);

	imo.imo_multicast_ifp = inm->inm_ifp;
	imo.imo_multicast_ttl = 1;
#ifdef RSVP_ISI
	imo.imo_multicast_vif = -1;
#endif
	/*
	 * Request loopback of the report if we are acting as a multicast
	 * router, so that the process-level routing demon can hear it.
	 */
#ifdef MROUTING
	imo.imo_multicast_loop = (ip_mrouter != NULL);
#else
	imo.imo_multicast_loop = 0;
#endif /* MROUTING */

	ip_output(m, (struct mbuf *)NULL, (struct route *)NULL,
	    IP_MULTICASTOPTS, &imo, (struct socket *)NULL);

	++igmpstat.igps_snd_reports;
}

void
igmp_purgeif(ifp)
	struct ifnet *ifp;
{
	rti_delete(ifp);
}
