/*	$NetBSD: npf_mbuf.c,v 1.6.14.3 2013/02/08 19:18:10 riz Exp $	*/

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
 * NPF network buffer management interface.
 *
 * Network buffer in NetBSD is mbuf.  Internal mbuf structures are
 * abstracted within this source.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_mbuf.c,v 1.6.14.3 2013/02/08 19:18:10 riz Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>

#include "nbpf.h"

/*
 * npf_addr_mask: apply the mask to a given address and store the result.
 */
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

/*
 * npf_cache_all: general routine to cache all relevant IP (v4 or v6)
 * and TCP, UDP or ICMP headers.
 */
int
nbpf_cache_all(nbpf_cache_t *npc, nbpf_buf_t *nbuf)
{
	void *nptr;

	nptr = nbpf_dataptr(nbuf);
	if (!nbpf_iscached(npc, NBPC_IP46) && !nbpf_fetch_ip(npc, nbuf, nptr)) {
		return (npc->bpc_info);
	}
	if (nbpf_iscached(npc, NBPC_IPFRAG)) {
		return (npc->bpc_info);
	}
	switch (nbpf_cache_ipproto(npc)) {
	case IPPROTO_TCP:
		(void)nbpf_fetch_tcp(npc, nbuf, nptr);
		break;
	case IPPROTO_UDP:
		(void)nbpf_fetch_udp(npc, nbuf, nptr);
		break;
	case IPPROTO_ICMP:
	case IPPROTO_ICMPV6:
		(void)nbpf_fetch_icmp(npc, nbuf, nptr);
		break;
	}
	return (npc->bpc_info);
}

void
nbpf_recache(nbpf_cache_t *npc, nbpf_buf_t *nbuf)
{
	const int mflags;
	int flags;

	mflags = npc->bpc_info & (NBPC_IP46 | NBPC_LAYER4);
	npc->bpc_info = 0;
	flags = nbpf_cache_all(npc, nbuf);
	KASSERT((flags & mflags) == mflags);
}

int
nbpf_reassembly(nbpf_cache_t *npc, nbpf_buf_t *nbuf, struct mbuf **mp)
{
	int error;

	if (nbpf_iscached(npc, NBPC_IP4)) {
		struct ip *ip = nbpf_dataptr(nbuf);
		error = ip_reass_packet(mp, ip);
	} else if (nbpf_iscached(npc, NBPC_IP6)) {
		error = ip6_reass_packet(mp, npc->bpc_hlen);
		if (error && *mp == NULL) {
			memset(nbuf, 0, sizeof(nbpf_buf_t));
		}
	}
	if (error) {
		return (error);
	}
	if (*mp == NULL) {
		return (0);
	}
	npc->bpc_info = 0;
	if (nbpf_cache_all(npc, nbuf) & NBPC_IPFRAG) {
		return (EINVAL);
	}
	return (0);
}

nbpf_cache_t *
nbpf_cache_init(struct mbuf **m)
{
	nbpf_buf_t nbuf;
	nbpf_cache_t *npc, np;
	int error;

	if (nbpf_cache_all(&np, &nbuf) &  NBPC_IPFRAG) {
		error = nbpf_reassembly(&np, &nbuf, m);
		if (error && np == NULL) {
			return (NULL);
		}
		if (m == NULL) {
			return (NULL);
		}
	}
	npc = &np;
	if (npc != NULL) {
		return (npc);
	}
	return (NULL);
}
