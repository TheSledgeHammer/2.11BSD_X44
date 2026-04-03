/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Contains two different approaches
 *
 * 1)
 * 		XOT: RFC1613
 * 		X.25 over TCP/IP (XOT)
 * 2)
 * 		XOT:
 * 		X.25 Tunnel over IP (Similar to ISO EON)
 * 		Will use the gif tunnel protocol
 */

#include <sys/cdefs.h>

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

#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <netccitt/x25.h>
#include <netccitt/pk.h>
#include <netccitt/pk_var.h>
#include <netccitt/xotvar.h>

#define IPPROTO_XOT 	40

void
xotattach()
{

}

static void
xot_ip_init(struct xot_iphdr *xip, struct in_addr src, struct in_addr dst, int ipproto, int zero)
{
	if (zero) {
		bzero((caddr_t)xip, sizeof(*xip));
	}
	xip->xi_src = src;
	xip->xi_dst = dst;
	xip->xi_p = ipproto;
}

static void
xot_tcp_init(struct xot_tcphdr *xtcp, int zero)
{
	if (zero) {
		bzero((caddr_t)xtcp, sizeof(*xtcp));
	}
	xtcp->xt_sport = XOT_TCPPORT;
	xtcp->xt_dport = XOT_TCPPORT;
}

static void
xot_hdr_init(struct xot_hdr *hdr, u_char xotver, u_char xotlen, int zero)
{
	if (zero) {
		bzero((caddr_t)hdr, sizeof(*hdr));
	}
	if (xotver != XOT_HDR_VERSION) {
		xotver = -1;
	}
	if (xotlen != XOT_HDR_LENGTH) {
		xotlen = -1;
	}
	hdr->xh_vers = xotver;
	hdr->xh_len = xotlen;
}

static void
xot_init(struct xot_packet *xot, struct in_addr src, struct in_addr dst, u_char xotver, u_char xotlen, int ipproto, int zero)
{
	xot_ip_init(&xot->xp_ip, src, dst, ipproto, zero);
	xot_tcp_init(&xot->xp_tcp, zero);
	xot_hdr_init(&xot->xp_hdr, xotver, xotlen, zero);
}

void
xot_packet_init(struct xot_packet *xot, struct sockaddr_in *src, struct sockaddr_in *dst, u_char xotver, u_char xotlen, int ipproto, int zero)
{
	if (src != NULL) {
		src->sin_family = AF_INET;
		src->sin_len = sizeof(*src);
	}
	if (dst != NULL) {
		dst->sin_family = AF_INET;
		dst->sin_len = sizeof(*dst);
	}
	xot_init(xot, src->sin_addr, dst->sin_addr, xotver, xotlen, ipproto, zero);
}

#ifdef notyet
struct tcphdr *
xot_attach(struct mbuf *m, struct socket *so, struct pklcd *lcd)
{
	struct tcphdr *th;
	int error;

	/* attach tcp to enter listen state */
	error = tcp_attach(so);
	if (error != 0) {
		return (NULL);
	}

	/* allocate tcphdr */
	th = mtod(m, sizeof(struct tcphdr *));
	if (th == NULL) {
		return (NULL);
	}

	/* attach x25 to enter listen state */
	lcd = pk_attach(so);
	if (lcd == NULL) {
		return (NULL);
	}
	return (th);
}

void
xot_detach(short state, short lcn)
{
	struct pklcd *lcd;

	/* lookup logical channel descriptor (lcn) */
	TAILQ_FOREACH(lcd, &pklcd_q, lcd_q) {
		/* check lcn state and number */
		if ((lcd->lcd_state == state) && (lcd->lcd_lcn == lcn)) {
			pk_close(lcd);
			if (lcd->lcd_state == LCN_ZOMBIE) {
				break;
			}
			break;
		}
	}
}
#endif

void
xotiphdr(struct xot_packet *xot, struct route *ro, int ipproto, int zero)
{
	struct rtentry *rt;
	struct mbuf     mhead;
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
	xot_packet_init(xot, NULL, sin, XOT_HDR_VERSION, XOT_HDR_LENGTH, ipproto, zero);
	mhead.m_data = (caddr_t)&xot->xp_hdr;
	mhead.m_len = sizeof(struct xot_hdr);
	mhead.m_next = 0;
}

void
xotrtrequest(int cmd, struct rtentry *rt, struct sockaddr *dst, int ipproto)
{
	struct llinfo_x25 *lx = (struct llinfo_x25 *)rt->rt_llinfo;

	x25_rtrequest2(cmd, rt, lx, dst);
	lx->lx_flags |= RTF_UP;
	xotiphdr(&lx->lx_xot, &lx->lx_route, ipproto, 0);
	if (lx->lx_route.ro_rt) {
		rt->rt_rmx.rmx_mtu = lx->lx_route.ro_rt->rt_rmx.rmx_mtu
				- sizeof(lx->lx_xot);
	}
}

/*
 * check xot_hdr isvalid:
 * Checks xot header version and length match the required xot version and length.
 */
bool_t
xot_hdr_isvalid(struct xot_packet *xot)
{
	/* check xot_hdr isvalid */
	if ((xot->xp_hdr.xh_vers != XOT_HDR_VERSION) || (xot->xp_hdr.xh_len != XOT_HDR_LENGTH)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * check tcp isvalid:
 * Checks tcp source and destination ports match xot required ports.
 * Checks tcp flags for syn packets.
 */
bool_t
xot_tcp_isvalid(struct xot_packet *xot)
{

	if ((xot->xp_tcp.xt_sport != XOT_TCPPORT)
			|| (xot->xp_tcp.xt_dport != XOT_TCPPORT)
			|| ((xot->xp_tcp.xt_flags & TH_SYN) == 0)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Generate Hash from network addr and network port
 */
u_int32_t
xot_nethash(void *addr, unsigned long *port)
{
	u_int32_t ahash = fnva_32_buf(addr, sizeof(addr), FNV1_32_INIT);
	u_int32_t phash = fnva_32_buf(port, sizeof(port), FNV1_32_INIT);
    return ((ahash + phash) % MAXNUMLCNS);
}

/*
 * Generate an X.25 LCN (logical channel number) from nethash
 * NOTE: Should only match outgoing interface for TCP/IP.
 */
void
xot_set_lcn(struct xot_packet *xot, void *addr, u_int16_t port)
{
	u_int32_t lcn = xot_nethash(addr, &port);
	SET_LCN(&xot->xp_x25p, lcn);
}

/* xot output: tcp/ip
 1. validate xot header
 xot header passed:
 2. setup tcp header (includes ip)
 3. create a tcp connection (includes ip)
 4. validate tcp header
tcp header passed:
 5. setup x25 packet (update ip)
	a: virtual circuits
	b: flow control (optional)
 6. create a x25 connection (update ip)
 7. tcp_output

 xot input: tcp/ip
 1. get tcp
 2. validate tcp header
 tcp header passed:
 3. get x.25 packet
 4. validate xot header
*/

int
xotoutput(struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst, struct rtentry *rt)
{
	struct sockaddr_x25 *sx25;
	struct llinfo_x25 *lx;
	struct xot_packet *xot;
	struct route *ro;
	int datalen;
	struct mbuf *mh;
	int error;

	sx25 = (struct sockaddr_x25 *)dst;
	error = 0;

	ifp->if_opackets++;
	xot = &lx->lx_xot;
	ro = &lx->lx_iproute;

	if ((lx->lx_flags & RTF_UP) == 0) {
		xotrtrequest(RTM_CHANGE, rt, dst, IPPROTO_XOT);
		if ((lx->lx_flags & RTF_UP) == 0) {
			return (EHOSTUNREACH);
		}
	}
	if ((m->m_flags & M_PKTHDR) == 0) {
		printf("xot: got non headered packet\n");
		return (EINVAL);
	}

	datalen = m->m_pkthdr.len + XOT_IPHLEN;
	if (datalen > IP_MAXPACKET) {
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
	m->m_pkthdr.len = datalen;
	xot->xp_ip.xi_len = htons(datalen);
	ifp->if_obytes += datalen;
	*mtod(m, struct xot_packet *) = *xot;

	error = ip_output(m, (struct mbuf *) 0, ro, 0, (struct ip_moptions *)NULL, (struct socket *)NULL);
	if (error) {
		ifp->if_oerrors++;
		ifp->if_ierrors = error;
	}
	return (error);
}

void
xotinput()
{

}
