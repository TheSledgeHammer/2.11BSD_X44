/*
 * nbpf_ops.c
 *
 *  Created on: 3 Feb 2024
 *      Author: marti
 */

#include "nbpf_impl.h"
#include "nbpf_ops.h"

struct nbpf_ops nbpf_ops = {
		.nbpf_match_ether = nbpf_ops_match_ether,
		.nbpf_match_proto = nbpf_ops_match_proto,
		.nbpf_match_ipv4 = nbpf_ops_match_ipv4,
		.nbpf_match_ipv6 = nbpf_ops_match_ipv6,
		.nbpf_match_tcp_ports = nbpf_ops_match_tcp_ports,
		.nbpf_match_udp_ports = nbpf_ops_match_udp_ports,
		.nbpf_match_icmp4 = nbpf_ops_match_icmp4,
		.nbpf_match_icmp6 = nbpf_ops_match_icmp6,
		.nbpf_match_tcpfl = nbpf_ops_match_tcpfl
};

static __inline int
nbpf_ops_match_ether(nbpf_buf_t *nbuf, int sd, int _res, uint16_t ethertype, uint32_t *r)
{
	return (nbpf_match_ether(nbuf, sd, _res, ethertype, r));
}

static __inline int
nbpf_ops_match_proto(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, uint32_t ap)
{
	nbpf_state_t *state = npc;
	return (nbpf_match_proto(state, nbuf, nptr, ap));
}

static __inline int
nbpf_ops_match_ipv4(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, const int szsd, const nbpf_in4_t *maddr, nbpf_netmask_t mask)
{
	nbpf_state_t *state = npc;
	return (nbpf_match_ipv4(state, nbuf, nptr, szsd, maddr, mask));
}

static __inline int
nbpf_ops_match_ipv6(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, const int szsd, const nbpf_in6_t *maddr, nbpf_netmask_t mask)
{
	nbpf_state_t *state = npc;
	return (nbpf_match_ipv6(state, nbuf, nptr, szsd, maddr, mask));
}

static __inline int
nbpf_ops_match_tcp_ports(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, const int sd, const uint32_t prange)
{
	nbpf_state_t *state = npc;
	return (nbpf_match_tcp_ports(state, nbuf, nptr, sd, prange));
}

static __inline int
nbpf_ops_match_udp_ports(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, const int sd, const uint32_t prange)
{
	nbpf_state_t *state = npc;
	return (nbpf_match_udp_ports(state, nbuf, nptr, sd, prange));
}

static __inline int
nbpf_ops_match_icmp4(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, uint32_t tc)
{
	nbpf_state_t *state = npc;
	return (nbpf_match_icmp4(state, nbuf, nptr, tc));
}

static __inline int
nbpf_ops_match_icmp6(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, uint32_t tc)
{
	nbpf_state_t *state = npc;
	return (nbpf_match_icmp6(state, nbuf, nptr, tc));
}

static __inline int
nbpf_ops_match_tcpfl(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, uint32_t fl)
{
	nbpf_state_t *state = npc;
	return (nbpf_match_tcpfl(state, nbuf, nptr, fl));
}
