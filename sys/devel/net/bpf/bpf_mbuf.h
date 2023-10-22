/*	$NetBSD: npf.h,v 1.14.2.12 2013/02/11 21:49:49 riz Exp $	*/

/*-
 * Copyright (c) 2009-2013 The NetBSD Foundation, Inc.
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

#ifndef _BPF_MBUF_H_
#define _BPF_MBUF_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stdbool.h>

#include <sys/ioctl.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>

/*
 * Packet information cache.
 */
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

#define	PACKET_TAG_NPF		10

typedef struct bpf_cache 	bpf_cache_t;
typedef struct nbuf 		nbuf_t;

/* Storage of address (both for IPv4 and IPv6) and netmask */
typedef struct in6_addr		bpf_addr_t;
typedef uint8_t				bpf_netmask_t;

#define	NPF_MAX_NETMASK		(128)
#define	NPF_NO_NETMASK		((npf_netmask_t)~0)

#define	NPC_IP4			0x01	/* Indicates fetched IPv4 header. */
#define	NPC_IP6			0x02	/* Indicates IPv6 header. */
#define	NPC_IPFRAG		0x04	/* IPv4/IPv6 fragment. */
#define	NPC_LAYER4		0x08	/* Layer 4 has been fetched. */

#define	NPC_TCP			0x10	/* TCP header. */
#define	NPC_UDP			0x20	/* UDP header. */
#define	NPC_ICMP		0x40	/* ICMP header. */
#define	NPC_ICMP_ID		0x80	/* ICMP with query ID. */

#define	NPC_ALG_EXEC	0x100	/* ALG execution. */

#define	NPC_IP46		(NPC_IP4|NPC_IP6)

struct bpf_cache {
    	/* Information flags. */
	uint32_t				bpc_info;
	/* Pointers to the IP v4/v6 addresses. */
	bpf_addr_t 				*bpc_srcip;
	bpf_addr_t 				*bpc_dstip;
	/* Size (v4 or v6) of IP addresses. */
	uint8_t					bpc_alen;
	uint8_t					bpc_hlen;
	uint16_t				bpc_proto;
	/* IPv4, IPv6. */
	union {
		struct ip 			*v4;
		struct ip6_hdr 		*v6;
	} bpc_ip;
	/* TCP, UDP, ICMP. */
	union {
		struct tcphdr 		*tcp;
		struct udphdr 		*udp;
		struct icmp 		*icmp;
		struct icmp6_hdr 	*icmp6;
		void 				*hdr;
	} bpc_l4;
};

static inline bool
bpf_iscached(const bpf_cache_t *bpc, const int inf)
{
	return __predict_true((bpc->bpc_info & inf) != 0);
}

/*
 * Network buffer interface.
 */

#define	NBUF_DATAREF_RESET	0x01

struct nbuf {
    struct mbuf 	    	*nb_mbuf0;
	struct mbuf 	    	*nb_mbuf;
	void 			    	*nb_nptr;
	const struct ifnet  	*nb_ifp;
	int		            	nb_flags;
};

void		nbuf_init(nbuf_t *, struct mbuf *, const struct ifnet *);
void		nbuf_reset(nbuf_t *);
struct mbuf *nbuf_head_mbuf(nbuf_t *);

bool		nbuf_flag_p(const nbuf_t *, int);
void		nbuf_unset_flag(nbuf_t *, int);

void 		*nbuf_dataptr(nbuf_t *);
size_t		nbuf_offset(const nbuf_t *);
void 		*nbuf_advance(nbuf_t *, size_t, size_t);

void 		*nbuf_ensure_contig(nbuf_t *, size_t);
void 		*nbuf_ensure_writable(nbuf_t *, size_t);

bool		nbuf_cksum_barrier(nbuf_t *, int);
int			nbuf_add_tag(nbuf_t *, uint32_t, uint32_t);
int			nbuf_find_tag(nbuf_t *, uint32_t, void **);


void 	*bpf_advance(struct mbuf *, void *, size_t, size_t);
bool 	bpf_cksum_barrier(struct mbuf *, int);
int 	bpf_add_tag(struct mbuf *, int, uint32_t, uint32_t);
int 	bpf_find_tag(struct mbuf *, int, uint32_t, void **);

#endif /* _BPF_MBUF_H_ */
