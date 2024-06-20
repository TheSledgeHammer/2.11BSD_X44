/*	$NetBSD: npf_instr.c,v 1.9.2.5.4.1 2013/11/17 19:21:21 bouyer Exp $	*/

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
 * NPF complex instructions.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_instr.c,v 1.9.2.7.2.1 2013/11/17 19:21:14 bouyer Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <net/if.h>
#include <net/ethertypes.h>
#include <net/if_ether.h>

#include "nbpf.h"

#define	NBPF_PORTRANGE_MATCH(r, p)	(p >= (r >> 16) && p <= (r & 0xffff))

/*
 * npf_match_ether: find and check Ethernet with possible VLAN headers.
 *
 * => Stores value in the register for advancing to layer 3 header.
 * => Returns zero on success or -1 on failure.
 */
int
nbpf_match_ether(nbpf_buf_t *nbuf, int sd, int _res, uint16_t ethertype, uint32_t *r)
{
	void *nptr;
	u_int offby;
	uint16_t val16;
	bool vlan;

	nptr = nbpf_dataptr(nbuf);
	vlan = false;
	*r = 0;

	/* Ethernet header: check EtherType. */
	offby = offsetof(struct ether_header, ether_type);
again:
	if (nbpf_advfetch(&nbuf, &nptr, offby, sizeof(uint16_t), &val16)) {
		return (-1);
	}
	val16 = ntohs(val16);
	*r += offby;

	/* Handle VLAN tags. */
	if (val16 == ETHERTYPE_VLAN && !vlan) {
		offby = sizeof(uint32_t);
		vlan = true;
		goto again;
	}
	if (val16 != ETHERTYPE_IP) {
		return (-1);
	}

	*r += ETHER_TYPE_LEN;
	return (0);
}

/*
 * npf_match_proto: match IP address length and/or layer 4 protocol.
 */
int
nbpf_match_proto(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, uint32_t ap)
{
	nbpf_ipv4_t *nb4 = &state->nbs_ip4;
	nbpf_ipv6_t *nb6 = &state->nbs_ip6;
	const int alen = (ap >> 8) & 0xff;
	const int proto = ap & 0xff;

	if (!nbpf_iscached(state, NBPC_IP46)) {
		/* Match an address against IPv4 */
		if (!nbpf_fetch_ipv4(state, nb4, nbuf, nptr)) {
			return (-1);
		}
		/* Match an address against IPv6 */
		if (!nbpf_fetch_ipv6(state, nb6, nbuf, nptr)) {
			return (-1);
		}
		KASSERT(nbpf_iscached(state, NBPC_IP46));
	}
	if (alen && state->nbs_alen != alen) {
		return (-1);
	}
	return ((proto != 0xff && nbpf_cache_ipproto(state) != proto) ? -1 : 0);
}

/*
 * npf_match_table: match IP address against NPF table.
 */
int
nbpf_match_table(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, const int sd, const u_int tid)
{
	nbpf_tableset_t *tblset;
	nbpf_ipv4_t *nb4 = &state->nbs_ip4;
	nbpf_ipv6_t *nb6 = &state->nbs_ip6;
	const nbpf_addr_t *addr, *addr4, addr6;
	const int alen;

	if (!nbpf_iscached(state, NBPC_IP46)) {
		/* Match an address against IPv4 */
		if (!nbpf_fetch_ipv4(state, nb4, nbuf, nptr)) {
			return (-1);
		}
		/* Match an address against IPv6 */
		if (!nbpf_fetch_ipv6(state, nb6, nbuf, nptr)) {
			return (-1);
		}
		KASSERT(nbpf_iscached(state, NBPC_IP46));
	}
	addr4 = (sd ? nb4->nb4_srcip : nb4->nb4_dstip);
	addr6 = (sd ? nb6->nb6_srcip : nb6->nb6_dstip);
	addr = (sd ? addr4 : addr6);
	//tblset = npf_core_tableset();
	alen = state->nbs_alen;

	return (nbpf_table_lookup(tblset, tid, alen, addr) ? -1 : 0);
}

/*
 * npf_match_ipmask: match an address against netaddr/mask.
 */
int
nbpf_match_ipv4(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, const int szsd, const nbpf_addr_t *maddr, nbpf_netmask_t mask)
{
	nbpf_ipv4_t *nb4 = &state->nbs_ip4;
	const int alen = szsd >> 1;
	nbpf_addr_t *addr;

	if (!nbpf_iscached(state, NBPC_IP4)) {
		if (!nbpf_fetch_ipv4(state, nb4, nbuf, nptr)) {
			return (-1);
		}
		KASSERT(nbpf_iscached(state, NBPC_IP4));
	}
	if (state->nbs_alen != alen) {
		return (-1);
	}
	return (nbpf_addr_cmp(maddr, NBPF_NO_NETMASK, addr, mask, alen) ? -1 : 0);
}

/*
 * npf_match_ipmask: match an address against netaddr/mask.
 */
int
nbpf_match_ipv6(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, const int szsd, const nbpf_addr_t *maddr, nbpf_netmask_t mask)
{
	nbpf_ipv6_t *nb6 = &state->nbs_ip6;
	const int alen = szsd >> 1;
	nbpf_addr_t *addr;

	if (!nbpf_iscached(state, NBPC_IP6)) {
		if (!nbpf_fetch_ipv6(state, nb6, nbuf, nptr)) {
			return (-1);
		}
		KASSERT(nbpf_iscached(state, NBPC_IP6));
	}
	if (state->nbs_alen != alen) {
		return (-1);
	}
	return (nbpf_addr_cmp(maddr, NBPF_NO_NETMASK, addr, mask, alen) ? -1 : 0);
}

/*
 * npf_match_tcp_ports: match TCP port in header against the range.
 */
int
nbpf_match_tcp_ports(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, const int sd, const uint32_t prange)
{
	nbpf_port_t *tcp = &state->nbs_port;
	struct tcphdr *th = &tcp->nbp_tcp;
	in_port_t p;

	if (!nbpf_iscached(state, NBPC_TCP)) {
		if (!nbpf_fetch_tcp(state, tcp, nbuf, nptr)) {
			return -1;
		}
		KASSERT(nbpf_iscached(state, NBPC_TCP));
	}
	p = sd ? th->th_sport : th->th_dport;

	/* Match against the port range. */
	return (NBPF_PORTRANGE_MATCH(prange, ntohs(p)) ? 0 : -1);
}

/*
 * npf_match_udp_ports: match UDP port in header against the range.
 */
int
nbpf_match_udp_ports(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, const int sd, const uint32_t prange)
{
	nbpf_port_t *udp = &state->nbs_port;
	struct udphdr *uh = &udp->nbp_udp;
	in_port_t p;

	if (!nbpf_iscached(state, NBPC_UDP)) {
		if (!nbpf_fetch_udp(state, udp, nbuf, nptr)) {
			return -1;
		}
		KASSERT(nbpf_iscached(state, NBPC_UDP));
	}
	p = sd ? uh->uh_sport : uh->uh_dport;

	/* Match against the port range. */
	return (NBPF_PORTRANGE_MATCH(prange, ntohs(p)) ? 0 : -1);
}

/*
 * nbpf_match_icmp4: match ICMPv4 packet.
 */
int
nbpf_match_icmp4(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, uint32_t tc)
{
	nbpf_icmp_t *icmp4 = &state->nbs_icmp;
	struct icmp *ic = &icmp4->nbi_icmp;

	if (!nbpf_iscached(state, NBPC_ICMP)) {
		if (!nbpf_fetch_icmp(state, icmp4, nbuf, nptr)) {
			return -1;
		}
		KASSERT(nbpf_iscached(state, NBPC_ICMP));
	}

	/* Match code/type, if required. */
	if ((1 << 31) & tc) {
		const uint8_t type = (tc >> 8) & 0xff;
		if (type != ic->icmp_type) {
			return (-1);
		}
	}
	if ((1 << 30) & tc) {
		const uint8_t code = tc & 0xff;
		if (code != ic->icmp_code) {
			return (-1);
		}
	}
	return (0);
}

/*
 * nbpf_match_icmp6: match ICMPv6 packet.
 */
int
nbpf_match_icmp6(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, uint32_t tc)
{
	nbpf_icmp_t *icmp6 = &state->nbs_icmp;
	struct icmp6_hdr *ic6 = &icmp6->nbi_icmp6;

	if (!nbpf_iscached(state, NBPC_ICMP)) {
		if (!nbpf_fetch_icmp(state, icmp6, nbuf, nptr)) {
			return -1;
		}
		KASSERT(nbpf_iscached(state, NBPC_ICMP));
	}

	/* Match code/type, if required. */
	if ((1 << 31) & tc) {
		const uint8_t type = (tc >> 8) & 0xff;
		if (type != ic6->icmp6_type) {
			return (-1);
		}
	}
	if ((1 << 30) & tc) {
		const uint8_t code = tc & 0xff;
		if (code != ic6->icmp6_code) {
			return (-1);
		}
	}
	return (0);
}

/*
 * nbpf_match_tcpfl: match TCP flags.
 */
int
nbpf_match_tcpfl(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, uint32_t fl)
{
	nbpf_port_t *tcp = &state->nbs_port;
	const uint8_t tcpfl = (fl >> 8) & 0xff, mask = fl & 0xff;
	struct tcphdr *th = &tcp->nbp_tcp;

	if (!nbpf_iscached(state, NBPC_TCP)) {
		if (!nbpf_fetch_tcp(state, tcp, nbuf, nptr)) {
			return (-1);
		}
		KASSERT(nbpf_iscached(state, NBPC_TCP));
	}
	return (((th->th_flags & mask) == tcpfl) ? 0 : -1);
}

void
nbpf_addr_mask(const nbpf_addr_t *addr, const nbpf_netmask_t mask, const int alen, nbpf_addr_t *out)
{
	const int nwords = alen >> 2;
	uint_fast8_t length = mask;

	/* Note: maximum length is 32 for IPv4 and 128 for IPv6. */
	KASSERT(length <= NBPF_MAX_NETMASK);

	for (int i = 0; i < nwords; i++) {
		uint32_t wordmask;

		if (length >= 32) {
			wordmask = htonl(0xffffffff);
			length -= 32;
		} else if (length) {
			wordmask = htonl(0xffffffff << (32 - length));
			length = 0;
		} else {
			wordmask = 0;
		}
		out->s6_addr32[i] = addr->s6_addr32[i] & wordmask;
	}
}

/*
 * npf_addr_cmp: compare two addresses, either IPv4 or IPv6.
 *
 * => Return 0 if equal and negative/positive if less/greater accordingly.
 * => Ignore the mask, if NPF_NO_NETMASK is specified.
 */
int
nbpf_addr_cmp(const nbpf_addr_t *addr1, const nbpf_netmask_t mask1, const nbpf_addr_t *addr2, const nbpf_netmask_t mask2, const int alen)
{
	nbpf_addr_t realaddr1, realaddr2;

	if (mask1 != NBPF_NO_NETMASK) {
		nbpf_addr_mask(addr1, mask1, alen, &realaddr1);
		addr1 = &realaddr1;
	}
	if (mask2 != NBPF_NO_NETMASK) {
		nbpf_addr_mask(addr2, mask2, alen, &realaddr2);
		addr2 = &realaddr2;
	}
	return (memcmp(addr1, addr2, alen));
}
