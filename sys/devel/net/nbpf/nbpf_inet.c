/*	$NetBSD: npf_inet.c,v 1.10.4.5.4.1 2012/12/16 18:20:09 riz Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Various procotol related helper routines.
 *
 * This layer manipulates npf_cache_t structure i.e. caches requested headers
 * and stores which information was cached in the information bit field.
 * It is also responsibility of this layer to update or invalidate the cache
 * on rewrites (e.g. by translation routines).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_inet.c,v 1.10.4.5.4.1 2012/12/16 18:20:09 riz Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <net/pfil.h>
#include <net/if.h>
#include <net/ethertypes.h>
#include <net/if_ether.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include "nbpf.h"

/*
 * npf_fetch_tcpopts: parse and return TCP options.
 */
bool
nbpf_fetch_tcpopts(const nbpf_cache_t *npc, nbpf_buf_t *nbuf, uint16_t *mss, int *wscale)
{
	void *nptr = nbuf_dataptr(nbuf);
	const struct tcphdr *th = &npc->bpc_l4.tcp;
	int topts_len, step;
	uint16_t val16;
	uint8_t val;

	KASSERT(nbpf_iscached(npc, NBPC_IP46));
	KASSERT(nbpf_iscached(npc, NBPC_TCP));

	/* Determine if there are any TCP options, get their length. */
	topts_len = (th->th_off << 2) - sizeof(struct tcphdr);
	if (topts_len <= 0) {
		/* No options. */
		return false;
	}
	KASSERT(topts_len <= MAX_TCPOPTLEN);

	/* First step: IP and TCP header up to options. */
	step = nbpf_cache_hlen(npc) + sizeof(struct tcphdr);
next:
	if (nbpf_advfetch(&nbuf, &nptr, step, sizeof(val), &val)) {
		return false;
	}

	switch (val) {
	case TCPOPT_EOL:
		/* Done. */
		return true;
	case TCPOPT_NOP:
		topts_len--;
		step = 1;
		break;
	case TCPOPT_MAXSEG:
		/*
		 * XXX: clean this mess.
		 */
		if (mss && *mss) {
			val16 = *mss;
			if (nbpf_advstore(&nbuf, &nptr, 2, sizeof(val16), &val16))
				return false;
		} else if (nbpf_advfetch(&nbuf, &nptr, 2, sizeof(val16), &val16)) {
			return false;
		}
		if (mss) {
			*mss = val16;
		}
		topts_len -= TCPOLEN_MAXSEG;
		step = sizeof(val16);
		break;
	case TCPOPT_WINDOW:
		/* TCP Window Scaling (RFC 1323). */
		if (nbuf_advfetch(&nbuf, &nptr, 2, sizeof(val), &val)) {
			return false;
		}
		*wscale = (val > TCP_MAX_WINSHIFT) ? TCP_MAX_WINSHIFT : val;
		topts_len -= TCPOLEN_WINDOW;
		step = sizeof(val);
		break;
	default:
		if (nbpf_advfetch(&nbuf, &nptr, 1, sizeof(val), &val)) {
			return false;
		}
		if (val < 2 || val > topts_len) {
			return false;
		}
		topts_len -= val;
		step = val - 1;
	}

	/* Any options left? */
	if (__predict_true(topts_len > 0)) {
		goto next;
	}
	return true;
}

/*
 * npf_fetch_ip: fetch, check and cache IP header.
 */
bool
nbpf_fetch_ip(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr)
{
	uint8_t ver;

	if (nbpf_fetch_datum(nbuf, nptr, sizeof(uint8_t), &ver)) {
		return false;
	}

	switch (ver >> 4) {
	case IPVERSION: {
		struct ip *ip = &npc->bpc_ip.v4;

		/* Fetch IPv4 header. */
		if (nbpf_fetch_datum(nbuf, nptr, sizeof(struct ip), ip)) {
			return false;
		}

		/* Check header length and fragment offset. */
		if ((u_int)(ip->ip_hl << 2) < sizeof(struct ip)) {
			return false;
		}
		if (ip->ip_off & ~htons(IP_DF | IP_RF)) {
			/* Note fragmentation. */
			npc->bpc_info |= NBPC_IPFRAG;
		}

		/* Cache: layer 3 - IPv4. */
		npc->bpc_alen = sizeof(struct in_addr);
		npc->bpc_srcip = (nbpf_addr_t *)&ip->ip_src;
		npc->bpc_dstip = (nbpf_addr_t *)&ip->ip_dst;
		npc->bpc_info |= NBPC_IP4;
		npc->bpc_hlen = ip->ip_hl << 2;
		npc->bpc_proto = npc->bpc_ip.v4->ip_p;
		break;
	}

	case (IPV6_VERSION >> 4): {
		struct ip6_hdr *ip6 = &npc->bpc_ip.v6;
		size_t hlen = sizeof(struct ip6_hdr);
		struct ip6_ext ip6e;

		/* Fetch IPv6 header and set initial next-protocol value. */
		if (nbpf_fetch_datum(nbuf, nptr, hlen, ip6)) {
			return false;
		}
		npc->bpc_proto = ip6->ip6_nxt;
		npc->bpc_hlen = hlen;

		/*
		 * Advance by the length of the current header and
		 * prefetch the extension header.
		 */
		while (nbpf_advfetch(&nbuf, &nptr, hlen, sizeof(struct ip6_ext), &ip6e) == 0) {
			/*
			 * Determine whether we are going to continue.
			 */
			switch (npc->bpc_proto) {
			case IPPROTO_HOPOPTS:
			case IPPROTO_DSTOPTS:
			case IPPROTO_ROUTING:
				hlen = (ip6e.ip6e_len + 1) << 3;
				break;
			case IPPROTO_FRAGMENT:
				npc->bpc_info |= NBPC_IPFRAG;
				hlen = sizeof(struct ip6_frag);
				break;
			case IPPROTO_AH:
				hlen = (ip6e.ip6e_len + 2) << 2;
				break;
			default:
				hlen = 0;
				break;
			}

			if (!hlen) {
				break;
			}
			npc->bpc_proto = ip6e.ip6e_nxt;
			npc->bpc_hlen += hlen;
		}

		/* Cache: layer 3 - IPv6. */
		npc->bpc_alen = sizeof(struct in6_addr);
		npc->bpc_srcip = (nbpf_addr_t *)&ip6->ip6_src;
		npc->bpc_dstip = (nbpf_addr_t *)&ip6->ip6_dst;
		npc->bpc_info |= NBPC_IP6;
		break;
	}
	default:
		return false;
	}

	return true;
}

/*
 * npf_fetch_tcp: fetch, check and cache TCP header.  If necessary,
 * fetch and cache layer 3 as well.
 */
bool
nbpf_fetch_tcp(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr)
{
	struct tcphdr *th;

	/* Must have IP header processed for its length and protocol. */
	if (!nbpf_iscached(npc, NBPC_IP46) && !nbpf_fetch_ip(npc, nbuf, nptr)) {
		return false;
	}
	if (nbpf_cache_ipproto(npc) != IPPROTO_TCP) {
		return false;
	}
	th = &npc->bpc_l4.tcp;

	/* Fetch TCP header. */
	if (nbpf_advfetch(&nbuf, &nptr, nbpf_cache_hlen(npc),
	    sizeof(struct tcphdr), th)) {
		return false;
	}

	/* Cache: layer 4 - TCP. */
	npc->bpc_info |= (NBPC_LAYER4 | NBPC_TCP);
	return true;
}

/*
 * npf_fetch_udp: fetch, check and cache UDP header.  If necessary,
 * fetch and cache layer 3 as well.
 */
bool
nbpf_fetch_udp(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr)
{
	struct udphdr *uh;
	u_int hlen;

	/* Must have IP header processed for its length and protocol. */
	if (!nbpf_iscached(npc, NBPC_IP46) && !nbpf_fetch_ip(npc, nbuf, nptr)) {
		return false;
	}
	if (nbpf_cache_ipproto(npc) != IPPROTO_UDP) {
		return false;
	}
	uh = &npc->bpc_l4.udp;
	hlen = nbpf_cache_hlen(npc);

	/* Fetch UDP header. */
	if (nbpf_advfetch(&nbuf, &nptr, hlen, sizeof(struct udphdr), uh)) {
		return false;
	}

	/* Cache: layer 4 - UDP. */
	npc->bpc_info |= (NBPC_LAYER4 | NBPC_UDP);
	return true;
}

/*
 * npf_fetch_icmp: fetch ICMP code, type and possible query ID.
 */
bool
nbpf_fetch_icmp(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr)
{
	struct icmp *ic;
	u_int hlen, iclen;

	/* Must have IP header processed for its length and protocol. */
	if (!nbpf_iscached(npc, NBPC_IP46) && !nbpf_fetch_ip(npc, nbuf, nptr)) {
		return false;
	}
	if (nbpf_cache_ipproto(npc) != IPPROTO_ICMP &&
	    nbpf_cache_ipproto(npc) != IPPROTO_ICMPV6) {
		return false;
	}
	ic = &npc->bpc_l4.icmp;
	hlen = nbpf_cache_hlen(npc);

	/* Fetch basic ICMP header, up to the "data" point. */
	CTASSERT(offsetof(struct icmp, icmp_void) == offsetof(struct icmp6_hdr, icmp6_data32));

	iclen = offsetof(struct icmp, icmp_void);
	if (nbpf_advfetch(&nbuf, &nptr, hlen, iclen, ic)) {
		return false;
	}

	/* Cache: layer 4 - ICMP. */
	npc->bpc_info |= (NBPC_LAYER4 | NBPC_ICMP);
	return true;
}
