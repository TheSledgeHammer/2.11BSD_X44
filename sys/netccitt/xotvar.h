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

#ifndef _NETCCITT_XOTVAR_H_
#define _NETCCITT_XOTVAR_H_

struct sockaddr_xot {

};

/* xot header */
struct xot_hdr {
	u_char 				xh_vers;	/* xot version number */
	u_char 				xh_len;		/* length of x25 packet */
};

#define XOT_HDR_VERSION 	0
#define XOT_HDR_LENGTH 		sizeof(struct x25_packet)

/* xot tcp */
struct xot_tcphdr {
	struct tcphdr		xt_tcp;
#define xt_sport 		xt_tcp.th_sport /* source port */
#define xt_dport 		xt_tcp.th_dport /* destination port */
#define xt_flags		xt_tcp.th_flags /* flags */
};

#define XOT_TCPPORT 	1998 /* tcp port number */
#define XOT_TCPHLEN 	(sizeof(struct xot_hdr) + sizeof(struct tcphdr)) /* xot_hdr + tcphdr header length */

/* xot ip */
struct xot_iphdr {
	struct ip			xi_ip;
#define xi_src 			xi_ip.ip_src	/* source ip address */
#define xi_dst			xi_ip.ip_dst	/* destination ip address */
#define xi_p			xi_ip.ip_p		/* ip protocol */
#define xi_len			xi_ip.ip_len	/* ip len */
#define xi_sum			xi_ip.ip_sum	/* ip sum */
#define xi_ttl			xi_ip.ip_ttl	/* ip ttl */
};

#define XOT_IPHLEN 		(sizeof(struct xot_hdr) + sizeof(struct ip)) /* xot_hdr + ip header length */
#define XOT_TCPIPHLEN	(sizeof(struct xot_hdr) + sizeof(struct tcphdr) + sizeof(struct ip))  /* xot_hdr + tcphdr + ip header length */

/* xot packet */
struct xot_packet {
	struct xot_iphdr	xp_ip;
	struct xot_tcphdr	xp_tcp;
	struct xot_hdr 		xp_hdr;
	struct x25_packet	xp_x25p;
};

#endif /* _NETCCITT_XOTVAR_H_ */
