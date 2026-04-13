/*	$NetBSD: if_eon.c,v 1.42 2003/09/30 00:01:18 christos Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
 *	@(#)if_eon.c	8.2 (Berkeley) 1/9/95
 */

#include <sys/cdefs.h>

#include "opt_ccitt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/netisr.h>
#include <net/route.h>

#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <netccitt/x25.h>
#include <netccitt/pk.h>
#include <netccitt/pk_var.h>
#include <netccitt/pk_extern.h>
#include <netccitt/xotvar.h>

struct ifnet  xotif[1];
struct xot_llinfo_head llinfo_xot;

void
xot_tun_attach(void)
{
	struct ifnet *ifp;

	ifp = xotif;
	sprintf(ifp->if_xname, "xot%d", 0);
	ifp->if_mtu = ETHERMTU;
	ifp->if_softc = NULL;
	/* since everything will go out over ether or token ring */
	ifp->if_ioctl = xot_tun_ioctl;
	ifp->if_output = xot_tun_output;
	ifp->if_type = IFT_X25;
	ifp->if_addrlen = 0;
	ifp->if_hdrlen = XOT_IPHLEN;
	ifp->if_flags = IFF_BROADCAST;
	if_attach(ifp);
	if_alloc_sadl(ifp);
	xot_tun_ioctl(ifp, SIOCSIFADDR, (caddr_t)TAILQ_FIRST(ifp->if_addrlist));
	LIST_INIT(&llinfo_xot);
}

int
xot_tun_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	int s = splnet();
	int error = 0;

	switch (cmd) {
		struct ifaddr *ifa;

	case SIOCSIFADDR:
		if ((ifa = (struct ifaddr *)data) != NULL) {
			ifp->if_flags |= IFF_UP;
			if (ifa->ifa_addr->sa_family != AF_LINK) {
				ifa->ifa_rtrequest = xotrtrequest;
			}
		}
		break;
	default:
		error = EINVAL;
		break;
	}
	splx(s);
	return (error);
}

void
xotiphdr(struct xot_packet *xot, struct route *ro, int zero)
{
	struct rtentry *rt;
	struct mbuf mhead;
	struct sockaddr_in *sin, *dst;

	sin = satosin(&ro->ro_dst);
	if (zero) {
		bzero((caddr_t)ro, sizeof(*ro));
	}
	if (ro->ro_rt) {
		dst = satosin(rt_key(ro->ro_rt));
		if ((ro->ro_rt->rt_flags & RTF_UP) == 0
				|| sin->sin_addr.s_addr != dst->sin_addr.s_addr) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)NULL;
		}
	}
	rtalloc(ro);
	if (ro->ro_rt) {
		ro->ro_rt->rt_use++;
	}
	mhead.m_data = (caddr_t)&xot->xp_hdr;
	mhead.m_len = sizeof(struct xot_hdr);
	mhead.m_next = 0;
	xot_packet_init(&mhead, xot, NULL, (struct sockaddr_in *)sin, XOT_HDR_VERSION, XOT_HDR_LENGTH, IPPROTO_XOT, XOT_IPHLEN, zero, AF_INET);
}

void
xotrtrequest(int cmd, struct rtentry *rt, struct rt_addrinfo *info)
{
	struct xot_llinfo *xl = (struct xot_llinfo *)rt->rt_llinfo;
	struct sockaddr *gate;

	switch (cmd) {
	case RTM_DELETE:
		if (xl) {
			if (xl->xl_route.ro_rt) {
				RTFREE(xl->xl_route.ro_rt);
			}
			Free(xl);
			rt->rt_llinfo = 0;
		}
		return;

	case RTM_ADD:
	case RTM_RESOLVE:
		rt->rt_rmx.rmx_mtu = loif[0].if_mtu;	/* unless better below */
		R_Malloc(xl, struct xot_llinfo *, sizeof(*xl));
		rt->rt_llinfo = (caddr_t)xl;
		if (xl == NULL) {
			return;
		}
		Bzero(xl, sizeof(*xl));
		LIST_INSERT_HEAD(&llinfo_xot, xl, xl_list);
		xl->xl_rt = rt;
		break;
	}
	if (info && (gate = info->rti_info[RTAX_GATEWAY])) {
		switch (gate->sa_family) {
		case AF_LINK:
			break;
		case AF_INET:
			break;
		default:
			return;
		}
	}
	xl->xl_flags |= RTF_UP;
	xotiphdr(&xl->xl_xot, &xl->xl_route, 0);
	if (xl->xl_route.ro_rt) {
		rt->rt_rmx.rmx_mtu = xl->xl_route.ro_rt->rt_rmx.rmx_mtu
				- sizeof(xl->xl_xot);
	}
}

int
xot_tun_output(struct ifnet *ifp, struct mbuf *m, struct sockaddr *sdst, struct rtentry *rt)
{
	struct sockaddr_x25 *dst;
	struct xot_llinfo *xl;
	struct xot_packet *xot;
	struct route *ro;
	struct mbuf *mh;
	int error, len, off;

	dst = (struct sockaddr_x25 *)sdst;
	xl = (struct xot_llinfo *)rt->rt_llinfo;
	if ((rt == NULL) || (xl == NULL)) {
		m_freem(m);
		return (EINVAL);
	}
	if ((xl->xl_flags & RTF_UP) == 0) {
		xotrtrequest(RTM_CHANGE, rt, (struct rt_addrinfo *)NULL);
		if ((xl->xl_flags & RTF_UP) == 0) {
			return (EHOSTUNREACH);
		}
	}
	if ((m->m_flags & M_PKTHDR) == 0) {
		printf("xot: got non headered packet\n");
		return (EINVAL);
	}

	xot = &xl->xl_xot;
	ro = &xl->xl_route;
	/* validate xot header */
	if (xot_hdr_isvalid(xot) != TRUE) {
		printf("xot_tun_output: xot header is invalid\n");
		return (EINVAL);
	}

	len = m->m_pkthdr.len + XOT_IPHLEN;
	if (len > IP_MAXPACKET) {
		m_freem(m);
		return (EMSGSIZE);
	}
	mh = m_gethdr(M_DONTWAIT, MT_HEADER);
	if (mh == 0) {
		m_freem(m);
		return (ENOBUFS);
	}
	mh->m_next = m;
	m = mh;
	MH_ALIGN(m, sizeof(struct xot_packet));
	m->m_len = sizeof(struct xot_packet);
	m->m_pkthdr.len = len;
	xot->xp_ip.xi_len = htons(len);
	ifp->if_obytes += len;
	*mtod(m, struct xot_packet *) = *xot;

	error = ip_output(m, (struct mbuf *)0, ro, 0, (struct ip_moptions *)NULL, (struct socket *)NULL);
	if (error) {
		ifp->if_oerrors++;
		ifp->if_ierrors = error;
	}
	return (error);
}

void
xot_tun_input(struct mbuf *m, ...)
{
	int iphlen;
	struct xot_packet *xot;
	struct ip *ip;
	struct ifnet *xotifp;
	int s;
	va_list ap;

	va_start(ap, m);
	iphlen = va_arg(ap, int);
	va_end(ap);

	xotifp = &xotif[0];

	if (m == NULL) {
		return;
	}
	if (iphlen > sizeof(struct ip)) {
		ip_stripoptions(m, (struct mbuf *)NULL);
	}
	if (m->m_len < XOT_IPHLEN) {
		m = m_pullup(m, XOT_IPHLEN);
		if (m == NULL) {
			m_freem(m);
			return;
		}
	}

	xotif->if_ibytes += m->m_pkthdr.len;
	ip = mtod(m, struct ip *);
	if (ip->ip_p != IPPROTO_XOT) {
		xotifp->if_ierrors++;
		m_freem(m);
		return;
	}
	m->m_data += sizeof(struct ip);
	xot = mtod(m, struct xot_packet *);
	/* validate xot header */
	if (xot_hdr_isvalid(xot) != TRUE) {
		xotifp->if_ierrors++;
		m_freem(m);
		return;
	}
	if (in4_cksum(m, IPPROTO_XOT, XOT_IPHLEN, m->m_pkthdr.len) != 0) {
		xotifp->if_ierrors++;
		m_freem(m);
		return;
	}
	m->m_data -= sizeof(struct ip);
	m->m_flags &= ~(M_BCAST | M_MCAST);
	xotifp->if_ipackets++;
	{
		/* put it on the PK queue and set soft interrupt */
		struct ifqueue *ifq;
		extern struct ifqueue pkintrq;

		m->m_pkthdr.rcvif = xotifp;	/* KLUDGE */

		ifq = &pkintrq;
		s = splnet();
		if (IF_QFULL(ifq)) {
			IF_DROP(ifq);
			m_freem(m);
			xotifp->if_iqdrops++;
			xotifp->if_ipackets--;
			splx(s);
			return;
		}
		IF_ENQUEUE(ifq, m);
		schednetisr(NETISR_CCITT);
		splx(s);
	}
}

void *
xot_tun_ctlinput(int cmd, struct sockaddr *sa, void *arg)
{
	switch (cmd) {
	case PRC_LINKUP:
		break;
	case PRC_LINKDOWN:
		break;
	case PRC_LINKRESET:
		break;
	case PRC_CONNECT_INDICATION:
	case PRC_DISCONNECT_INDICATION:
	}
	return (NULL);
}
