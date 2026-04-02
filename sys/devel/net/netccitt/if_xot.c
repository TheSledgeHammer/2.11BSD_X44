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

/* XOT: RFC1613
 * X.25 over TCP/IP (XOT)
 */

#include <sys/malloc.h>

#include <net/route.h>

#include <netccitt/x25.h>
#include <netccitt/xotvar.h>

#include <netinet/tcp.h>

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
xotiphdr(struct xot_packet *xot, struct route *ro, int zero)
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
	xot_packet_init(xot, NULL, sin, XOT_HDR_VERSION, XOT_HDR_LENGTH, zero);
	mhead.m_data = (caddr_t)&xot->xp_hdr;
	mhead.m_len = sizeof(struct xot_hdr);
	mhead.m_next = 0;
}

void
xotrtrequest(int cmd, struct rtentry *rt, struct sockaddr *dst)
{
	struct llinfo_x25 *lx = (struct llinfo_x25 *)rt->rt_llinfo;

	x25_rtrequest2(cmd, rt, lx, dst);
	xotiphdr(&lx->lx_xot, &lx->lx_route, 0);
	if (lx->lx_route.ro_rt) {
		rt->rt_rmx.rmx_mtu = lx->lx_route.ro_rt->rt_rmx.rmx_mtu
				- sizeof(lx->lx_xot);
	}
}

int
xotoutput()
{

}

void
xotinput()
{

}
