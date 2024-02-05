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

#include "nbpf_impl.h"

/*
 * npf_fetch_ip4: fetch, check and cache IPv6 header.
 */
bool
nbpf_fetch_ipv4(nbpf_state_t *state, nbpf_ipv4_t *nb4, nbpf_buf_t *nbuf, void *nptr)
{
	struct ip *ip = &nb4->nb4_v4;

	/* Fetch IPv4 header. */
	if (nbpf_fetch_datum(nbuf, nptr, sizeof(struct ip), ip)) {
		return (false);
	}
	/* Check header length and fragment offset. */
	if ((u_int)(ip->ip_hl << 2) < sizeof(struct ip)) {
		return (false);
	}
	if (ip->ip_off & ~htons(IP_DF | IP_RF)) {
		/* Note fragmentation. */
		state->nbs_info |= NBPC_IPFRAG;
	}
	state->nbs_alen = sizeof(struct in_addr);
	nb4->nb4_srcip = (nbpf_in4_t *)&ip->ip_src;
	nb4->nb4_dstip = (nbpf_in4_t *)&ip->ip_dst;
	state->nbs_info |= NBPC_IP4;
	state->nbs_hlen = ip->ip_hl << 2;
	state->nbs_proto = nb4->nb4_v4->ip_p;
	*state->nbs_ip4 = nb4;
	return (true);
}

/*
 * npf_fetch_ip6: fetch, check and cache IPv6 header.
 */
bool
nbpf_fetch_ipv6(nbpf_state_t *state, nbpf_ipv6_t *nb6, nbpf_buf_t *nbuf, void *nptr)
{
	struct ip6_hdr *ip6 = &nb6->nb6_v6;
	size_t hlen = sizeof(struct ip6_hdr);
	struct ip6_ext ip6e;

	/* Fetch IPv6 header and set initial next-protocol value. */
	if (nbpf_fetch_datum(nbuf, nptr, hlen, ip6)) {
		return (false);
	}

	state->nbs_proto = ip6->ip6_nxt;
	state->nbs_hlen = hlen;

	while (nbpf_advfetch(&nbuf, &nptr, hlen, sizeof(struct ip6_ext), &ip6e) == 0) {
		/*
		 * Determine whether we are going to continue.
		 */
		switch (state->nbs_proto) {
		case IPPROTO_HOPOPTS:
		case IPPROTO_DSTOPTS:
		case IPPROTO_ROUTING:
			hlen = (ip6e.ip6e_len + 1) << 3;
			break;
		case IPPROTO_FRAGMENT:
			state->nbs_info |= NBPC_IPFRAG;
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
		state->nbs_proto = ip6e.ip6e_nxt;
		state->nbs_hlen += hlen;
	}
	state->nbs_alen = sizeof(struct in6_addr);
	nb6->nb6_srcip = (nbpf_in6_t *)&ip6->ip6_src;
	nb6->nb6_dstip = (nbpf_in6_t *)&ip6->ip6_dst;
	state->nbs_info |= NBPC_IP6;
	*state->nbs_ip6 = nb6;
	return (true);
}

/*
 * npf_fetch_tcp: fetch, check and cache TCP header.  If necessary,
 * fetch and cache layer 3 as well.
 */
bool
nbpf_fetch_tcp(nbpf_state_t *state, nbpf_port_t *tcp, nbpf_buf_t *nbuf, void *nptr)
{
	struct tcphdr *th;

	/* Must have IP header processed for its length and protocol. */
	if (!nbpf_iscached(state, NBPC_IP46) &&
			!nbpf_fetch_ipv4(state, &state->nbs_ip4, nbuf, nptr) &&
			!nbpf_fetch_ipv6(state, &state->nbs_ip6, nbuf, nptr)) {
		return (false);
	}

	if (nbpf_cache_ipproto(state) != IPPROTO_TCP) {
		return (false);
	}

	th = &tcp->nbp_tcp;

	/* Fetch TCP header. */
	if (nbpf_advfetch(&nbuf, &nptr, nbpf_cache_hlen(state), sizeof(struct tcphdr), th)) {
		return (false);
	}

	/* Cache: layer 4 - TCP. */
	state->nbs_info |= (NBPC_LAYER4 | NBPC_TCP);
	*state->nbs_port = tcp;
	return (true);
}

/*
 * npf_fetch_udp: fetch, check and cache UDP header.  If necessary,
 * fetch and cache layer 3 as well.
 */
bool
nbpf_fetch_udp(nbpf_state_t *state, nbpf_port_t *udp, nbpf_buf_t *nbuf, void *nptr)
{
	struct udphdr *uh;

	/* Must have IP header processed for its length and protocol. */
	if (!nbpf_iscached(state, NBPC_IP46) &&
			!nbpf_fetch_ipv4(state, &state->nbs_ip4, nbuf, nptr) &&
			!nbpf_fetch_ipv6(state, &state->nbs_ip6, nbuf, nptr)) {
		return (false);
	}
	if (nbpf_cache_ipproto(state) != IPPROTO_UDP) {
		return (false);
	}

	uh = &udp->nbp_udp;

	/* Fetch UDP header. */
	if (nbpf_advfetch(&nbuf, &nptr, nbpf_cache_hlen(state), sizeof(struct udphdr), uh)) {
		return (false);
	}

	/* Cache: layer 4 - UDP. */
	state->nbs_info |= (NBPC_LAYER4 | NBPC_UDP);
	*state->nbs_port = udp;
	return (true);
}

/*
 * npf_fetch_icmp: fetch ICMP code, type and possible query ID.
 */
bool
nbpf_fetch_icmp(nbpf_state_t *state, nbpf_icmp_t *icmp, nbpf_buf_t *nbuf, void *nptr)
{
	struct icmp *ic;
	u_int iclen;

	/* Must have IP header processed for its length and protocol. */
	if (!nbpf_iscached(state, NBPC_IP46) &&
			!nbpf_fetch_ipv4(state, &state->nbs_ip4, nbuf, nptr) &&
			!nbpf_fetch_ipv6(state, &state->nbs_ip6, nbuf, nptr)) {
		return (false);
	}
	if (nbpf_cache_ipproto(state) != IPPROTO_ICMP &&
	    nbpf_cache_ipproto(state) != IPPROTO_ICMPV6) {
		return (false);
	}
	ic = &icmp->nbi_icmp;

	/* Fetch basic ICMP header, up to the "data" point. */
	CTASSERT(offsetof(struct icmp, icmp_void) == offsetof(struct icmp6_hdr, icmp6_data32));

	iclen = offsetof(struct icmp, icmp_void);
	if (nbpf_advfetch(&nbuf, &nptr, nbpf_cache_hlen(state), iclen, ic)) {
		return (false);
	}

	/* Cache: layer 4 - ICMP. */
	state->nbs_info |= (NBPC_LAYER4 | NBPC_ICMP);
	*state->nbs_icmp = icmp;
	return (true);
}

/*
 * npf_fetch_tcpopts: parse and return TCP options.
 */
bool
nbpf_fetch_tcpopts(const nbpf_state_t *state, nbpf_port_t *tcp, nbpf_buf_t *nbuf, uint16_t *mss, int *wscale)
{
	void *nptr = nbuf_dataptr(nbuf);
	const struct tcphdr *th = &tcp->nbp_tcp;
	int topts_len, step;
	uint16_t val16;
	uint8_t val;

	KASSERT(nbpf_iscached(state, NBPC_IP46));
	KASSERT(nbpf_iscached(state, NBPC_TCP));

	/* Determine if there are any TCP options, get their length. */
	topts_len = (th->th_off << 2) - sizeof(struct tcphdr);
	if (topts_len <= 0) {
		/* No options. */
		return (false);
	}
	KASSERT(topts_len <= MAX_TCPOPTLEN);

	/* First step: IP and TCP header up to options. */
	step = nbpf_cache_hlen(state) + sizeof(struct tcphdr);
next:
	if (nbpf_advfetch(&nbuf, &nptr, step, sizeof(val), &val)) {
		return (false);
	}

	switch (val) {
	case TCPOPT_EOL:
		/* Done. */
		return (true);
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
				return (false);
		} else if (nbpf_advfetch(&nbuf, &nptr, 2, sizeof(val16), &val16)) {
			return (false);
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
			return (false);
		}
		*wscale = (val > TCP_MAX_WINSHIFT) ? TCP_MAX_WINSHIFT : val;
		topts_len -= TCPOLEN_WINDOW;
		step = sizeof(val);
		break;
	default:
		if (nbpf_advfetch(&nbuf, &nptr, 1, sizeof(val), &val)) {
			return (false);
		}
		if (val < 2 || val > topts_len) {
			return (false);
		}
		topts_len -= val;
		step = val - 1;
	}

	/* Any options left? */
	if (__predict_true(topts_len > 0)) {
		goto next;
	}
	return (true);
}
