/*	$NetBSD: npf_ncode.h,v 1.5.6.5 2013/02/11 21:49:49 riz Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
 * NBPF: A BPF Extension (Based of the NPF ncode from NetBSD 6.0/6.1)
 * Goals:
 * - Use most of the existing BPF framework
 * - Works with BPF not instead of.
 * - Most Important: Extend upon filter capabilities
 */

#ifndef _NET_NBPF_H_
#define _NET_NBPF_H_

#include <sys/param.h>
#include <sys/systm.h>
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

/* Storage of address (both for IPv4 and IPv6) and netmask */
typedef struct in6_addr		nbpf_addr_t;
typedef uint8_t				nbpf_netmask_t;

typedef union nbpf_ipv4 	nbpf_ipv4_t;
typedef union nbpf_ipv6 	nbpf_ipv6_t;
typedef union nbpf_port 	nbpf_port_t;
typedef union nbpf_icmp 	nbpf_icmp_t;

typedef struct nbpf_table	nbpf_table_t;
typedef struct nbpf_tblent 	nbpf_tblent_t;
typedef nbpf_table_t 		*nbpf_tableset_t;

typedef void 				nbpf_buf_t;
typedef void 				nbpf_cache_t;
typedef struct nbpf_state	nbpf_state_t;
typedef int 				nbpf_tag_t;

#define	NBPF_MAX_NETMASK	(128)
#define	NBPF_NO_NETMASK		((nbpf_netmask_t)~0)

#define	NBPC_IP4			0x01	/* Indicates fetched IPv4 header. */
#define	NBPC_IP6			0x02	/* Indicates IPv6 header. */
#define	NBPC_IPFRAG			0x04	/* IPv4/IPv6 fragment. */
#define	NBPC_LAYER4			0x08	/* Layer 4 has been fetched. */

#define	NBPC_TCP			0x10	/* TCP header. */
#define	NBPC_UDP			0x20	/* UDP header. */
#define	NBPC_ICMP			0x40	/* ICMP header. */

#define	NBPC_IP46			(NBPC_IP4|NBPC_IP6)

union nbpf_icmp {
	nbpf_state_t		*nbi_state;
	struct icmp 		nbi_icmp;
	struct icmp6_hdr 	nbi_icmp6;
};

union nbpf_port {
	nbpf_state_t		*nbp_state;
	struct tcphdr 		nbp_tcp;
	struct udphdr 		nbp_udp;
};

union nbpf_ipv4 {
	nbpf_state_t		*nb4_state;
	/* Pointers to the IP v4 addresses. */
	nbpf_addr_t 		*nb4_srcip;
	nbpf_addr_t 		*nb4_dstip;
	struct ip 			nb4_v4;
};

union nbpf_ipv6 {
	nbpf_state_t		*nb6_state;
	/* Pointers to the IP v6 addresses. */
	nbpf_addr_t			*nb6_srcip;
	nbpf_addr_t			*nb6_dstip;
	struct ip6_hdr 		nb6_v6;
};

struct nbpf_state {
	/* Information flags. */
	uint32_t 			nbs_info;
	/* mbuf tag */
	nbpf_tag_t			nbs_tag;
	/* Size (v4 & v6) of IP addresses. */
	uint8_t				nbs_alen;
	uint8_t				nbs_hlen;
	uint16_t			nbs_proto;
	/* IPv4, IPv6. */
	nbpf_ipv4_t			nbs_ip4;
	nbpf_ipv6_t			nbs_ip6;
	/* TCP, UDP, ICMP. */
	nbpf_port_t			nbs_port;
	nbpf_icmp_t			nbs_icmp;
};

static inline bool_t
nbpf_iscached(const nbpf_state_t *state, const int inf)
{
	return __predict_true((state->nbs_info & inf) != 0);
}

static inline int
nbpf_cache_ipproto(const nbpf_state_t *state)
{
	KASSERT(nbpf_iscached(state, NBPC_IP46));
	return (state->nbs_proto);
}

static inline u_int
nbpf_cache_hlen(const nbpf_state_t *state)
{
	KASSERT(nbpf_iscached(state, NBPC_IP46));
	return (state->nbs_hlen);
}

static inline void
nbpf_set_tag(const nbpf_state_t *state, int tag)
{
	state->nbs_tag = tag;
}

static inline int
nbpf_get_tag(const nbpf_state_t *state)
{
	return (state->nbs_tag);
}

static __inline void
nbpf_malloc(cache, size, type, flags)
	void *cache;
	u_long size;
	int type, flags;
{
	cache = malloc(size, type, flags);
}

static __inline void
nbpf_free(cache, type)
	void *cache;
	int type;
{
	free(cache, type);
}

/* Network buffer interface. */
void 	*nbpf_dataptr(void *);
void 	*nbpf_advance(nbpf_buf_t **, void *, u_int);
int		nbpf_advfetch(nbpf_buf_t **, void **, u_int, size_t, void *);
int		nbpf_advstore(nbpf_buf_t **, void **, u_int, size_t, void *);
int		nbpf_fetch_datum(nbpf_buf_t *, void *, size_t, void *);
int		nbpf_store_datum(nbpf_buf_t *, void *, size_t, void *);
bool_t	nbpf_cksum_barrier(nbpf_buf_t *, int);
int		nbpf_add_tag(nbpf_state_t *, nbpf_buf_t *, uint32_t, uint32_t);
int		nbpf_find_tag(nbpf_state_t *, nbpf_buf_t *, uint32_t, void **);

/* Protocol helpers. */
bool	nbpf_fetch_ipv4(nbpf_state_t *, nbpf_ipv4_t *, nbpf_buf_t *, void *);
bool	nbpf_fetch_ipv6(nbpf_state_t *, nbpf_ipv6_t *, nbpf_buf_t *, void *);
bool	nbpf_fetch_tcp(nbpf_state_t *, nbpf_port_t *, nbpf_buf_t *, void *);
bool	nbpf_fetch_udp(nbpf_state_t *, nbpf_port_t *, nbpf_buf_t *, void *);
bool	nbpf_fetch_icmp(nbpf_state_t *, nbpf_icmp_t *, nbpf_buf_t *, void *);
bool	nbpf_fetch_tcpopts(const nbpf_state_t *, nbpf_port_t *, nbpf_buf_t *, uint16_t *, int *);

int		nbpf_cache_all(nbpf_state_t *, nbpf_buf_t *);
void	nbpf_recache(nbpf_state_t *, nbpf_buf_t *);
int		nbpf_reassembly(nbpf_state_t *, nbpf_buf_t *, struct mbuf **);

int		nbpf_addr_cmp(const nbpf_addr_t *, const nbpf_netmask_t, const nbpf_addr_t *, const nbpf_netmask_t, const int);
void	nbpf_addr_mask(const nbpf_addr_t *, const nbpf_netmask_t, const int, nbpf_addr_t *);

/* Complex instructions. */
int		nbpf_match_ether(nbpf_buf_t *, int, int, uint16_t, uint32_t *);
int		nbpf_match_proto(nbpf_state_t *, nbpf_buf_t *, void *, uint32_t);
int		nbpf_match_table(nbpf_state_t *, nbpf_buf_t *, void *, const int, const u_int);
int		nbpf_match_ipv4(nbpf_state_t *, nbpf_buf_t *, void *, const int, const nbpf_addr_t *, const nbpf_netmask_t);
int		nbpf_match_ipv6(nbpf_state_t *, nbpf_buf_t *, void *, const int, const nbpf_addr_t *, const nbpf_netmask_t);
int		nbpf_match_tcp_ports(nbpf_state_t *, nbpf_buf_t *, void *, const int, const uint32_t);
int		nbpf_match_udp_ports(nbpf_state_t *, nbpf_buf_t *, void *, const int, const uint32_t);
int		nbpf_match_icmp4(nbpf_state_t *, nbpf_buf_t *, void *, uint32_t);
int		nbpf_match_icmp6(nbpf_state_t *, nbpf_buf_t *, void *, uint32_t);
int		nbpf_match_tcpfl(nbpf_state_t *, nbpf_buf_t *, void *, uint32_t);

/* Tableset interface. */
void	nbpf_tableset_sysinit(void);
void	nbpf_tableset_sysfini(void);

extern const pt_tree_ops_t nbpf_table_ptree_ops;

nbpf_tableset_t *nbpf_tableset_create(void);
void	nbpf_tableset_destroy(nbpf_tableset_t *);
int		nbpf_tableset_insert(nbpf_tableset_t *, nbpf_table_t *);
void	nbpf_tableset_reload(nbpf_tableset_t *, nbpf_tableset_t *);

nbpf_table_t *nbpf_table_create(u_int, int, size_t);
void		nbpf_table_destroy(nbpf_table_t *);

int		nbpf_table_check(const nbpf_tableset_t *, u_int, int);
int		nbpf_table_insert(nbpf_tableset_t *, u_int, const int, const nbpf_addr_t *, const nbpf_netmask_t);
int		nbpf_table_remove(nbpf_tableset_t *, u_int, const int, const nbpf_addr_t *, const nbpf_netmask_t);
int		nbpf_table_lookup(nbpf_tableset_t *, u_int, const int, const nbpf_addr_t *);
int		nbpf_table_list(nbpf_tableset_t *, u_int, void *, size_t);
#endif /* _NET_NBPF_H_ */
