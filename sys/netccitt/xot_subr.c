/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

static void
xot_ip4_init(struct mbuf *m, struct xot_iphdr *xip, struct in_addr src, struct in_addr dst, int ipproto, u_int16_t iphlen, int zero)
{
	xip->xi_p = ipproto;
	xip->xi_ttl = MAXTTL;
	xip->xi_len = iphlen;
	xip->xi_sum = in4_cksum(m, ipproto, sizeof(struct ip), m->m_pkthdr.len);
	xip->xi_src = src;
	xip->xi_dst = dst;
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
xot_init(struct mbuf *m, struct xot_packet *xot, struct in_addr src, struct in_addr dst, u_char xotver, u_char xotlen, int ipproto, u_int16_t iphlen, int zero)
{
	xot_ip4_init(m, &xot->xp_ip, src, dst, ipproto, iphlen, zero);
#ifndef XOT_TUN
	xot_tcp_init(&xot->xp_tcp, zero);
#endif /* XOT_TUN */
	xot_hdr_init(&xot->xp_hdr, xotver, xotlen, zero);
	xot_mbuf_init(m, xot);
}

void
xot_packet_init(struct mbuf *m, struct xot_packet *xot, struct sockaddr_in *src, struct sockaddr_in *dst, u_char xotver, u_char xotlen, int ipproto, u_int16_t iphlen, int zero)
{
	if (src != NULL) {
		src->sin_family = AF_INET;
		src->sin_len = sizeof(*src);
	}
	if (dst != NULL) {
		dst->sin_family = AF_INET;
		dst->sin_len = sizeof(*dst);
	}
	xot_init(m, xot, src->sin_addr, dst->sin_addr, xotver, xotlen, ipproto, iphlen, zero);
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
