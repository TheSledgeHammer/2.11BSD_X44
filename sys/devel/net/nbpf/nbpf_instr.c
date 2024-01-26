/*	$NetBSD: npf_instr.c,v 1.9.2.7.2.1 2013/11/17 19:21:14 bouyer Exp $	*/

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
#include <netinet/in_systm.h>
#include <netinet/in.h>

#include "nbpf.h"

#define	NBPF_PORTRANGE_MATCH(r, p)	(p >= (r >> 16) && p <= (r & 0xffff))

/*
 * npf_match_ether: find and check Ethernet with possible VLAN headers.
 *
 * => Stores value in the register for advancing to layer 3 header.
 * => Returns zero on success or -1 on failure.
 */
int
nbpf_match_ether(struct mbuf *m, int sd, uint16_t ethertype, uint32_t *r)
{
	const u_int off;
	bool vlan = false;
	void *nptr;
	u_int offby;
	uint16_t val16;

	off = nbpf_offset(m);
	/* Ethernet header: check EtherType. */
	offby = offsetof(struct ether_header, ether_type);
	*r = 0;
again:
	if ((nptr = nbpf_advance(m, offby)) == NULL) {
		return (-1);
	}
	memcpy(&val16, nptr, sizeof(val16));
	*r += offby;

	/* Handle VLAN tags. */
	if (val16 == ETHERTYPE_VLAN && !vlan) {
		offby = sizeof(uint32_t);
		vlan = true;
		goto again;
	}

	/* Restore the offset. */
	nbpf_advance(m, off);

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
nbpf_match_proto(const nbpf_cache_t *npc, uint32_t ap)
{
	const int alen = (ap >> 8) & 0xff;
	const int proto = ap & 0xff;

	KASSERT(nbpf_iscached(npc, NBPC_IP46));
	if (alen && npc->bpc_alen != alen) {
		return -1;
	}
	return ((proto != 0xff && npc->bpc_proto != proto) ? -1 : 0);
}

/*
 * npf_match_table: match IP address against NPF table.
 */
int
nbpf_match_table(const nbpf_cache_t *npc, int sd, u_int tid)
{
	npf_tableset_t *tblset = npf_config_tableset();
	const nbpf_addr_t *addr = sd ? npc->bpc_srcip : npc->bpc_dstip;
	const int alen = npc->bpc_alen;

	KASSERT(nbpf_iscached(npc, NBPC_IP46));

	/* Match address against NPF table. */
	return (npf_table_lookup(tblset, tid, alen, addr) ? -1 : 0);
}

/*
 * npf_match_ipmask: match an address against netaddr/mask.
 */
int
nbpf_match_ipmask(const nbpf_cache_t *npc, int szsd, const nbpf_addr_t *maddr, nbpf_netmask_t mask)
{
	const int alen = szsd >> 1;
	const npf_addr_t *addr;

	KASSERT(nbpf_iscached(npc, NBPC_IP46));
	if (npc->bpc_alen != alen) {
		return -1;
	}
	addr = (szsd & 0x1) ? npc->bpc_srcip : npc->bpc_dstip;
	return (nbpf_addr_cmp(maddr, NBPF_NO_NETMASK, addr, mask, alen) ? -1 : 0);
}

/*
 * npf_match_tcp_ports: match TCP port in header against the range.
 */
int
nbpf_match_tcp_ports(const nbpf_cache_t *npc, int sd, uint32_t prange)
{
	const struct tcphdr *th = npc->bpc_l4.tcp;
	const in_port_t p = sd ? th->th_sport : th->th_dport;

	KASSERT(nbpf_iscached(npc, NBPC_TCP));

	/* Match against the port range. */
	return (NBPF_PORTRANGE_MATCH(prange, ntohs(p)) ? 0 : -1);
}

/*
 * npf_match_udp_ports: match UDP port in header against the range.
 */
int
nbpf_match_udp_ports(const nbpf_cache_t *npc, int sd, uint32_t prange)
{
	const struct udphdr *uh = npc->bpc_l4.udp;
	const in_port_t p = sd ? uh->uh_sport : uh->uh_dport;

	KASSERT(nbpf_iscached(npc, NBPC_UDP));

	/* Match against the port range. */
	return (NBPF_PORTRANGE_MATCH(prange, ntohs(p)) ? 0 : -1);
}

/*
 * npf_match_icmp4: match ICMPv4 packet.
 */
int
nbpf_match_icmp4(const nbpf_cache_t *npc, uint32_t tc)
{
	const struct icmp *ic = npc->bpc_l4.icmp;

	KASSERT(nbpf_iscached(npc, NBPC_ICMP));

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
 * npf_match_icmp6: match ICMPv6 packet.
 */
int
nbpf_match_icmp6(const nbpf_cache_t *npc, uint32_t tc)
{
	const struct icmp6_hdr *ic6 = npc->bpc_l4.icmp6;

	KASSERT(nbpf_iscached(npc, NBPC_ICMP));

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
 * npf_match_tcpfl: match TCP flags.
 */
int
nbpf_match_tcpfl(const nbpf_cache_t *npc, uint32_t fl)
{
	const uint8_t tcpfl = (fl >> 8) & 0xff, mask = fl & 0xff;
	const struct tcphdr *th = npc->bpc_l4.tcp;

	KASSERT(nbpf_iscached(npc, NBPC_TCP));
	return ((th->th_flags & mask) == tcpfl ? 0 : -1);
}
