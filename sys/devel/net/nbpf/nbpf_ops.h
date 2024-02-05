/*
 * nbpf_ops.h
 *
 *  Created on: 3 Feb 2024
 *      Author: marti
 */

#ifndef SYS_DEVEL_NET_NBPF_V2_NBPF_OPS_H_
#define SYS_DEVEL_NET_NBPF_V2_NBPF_OPS_H_

struct nbpf_ops {
	int (*nbpf_match_ether)(nbpf_buf_t *, int, int, uint16_t, uint32_t *);
	int (*nbpf_match_proto)(nbpf_cache_t *, nbpf_buf_t *, void *, uint32_t);
	int (*nbpf_match_ipv4)(nbpf_cache_t *, nbpf_buf_t *, void *, const int, const nbpf_in4_t *, nbpf_netmask_t);
	int (*nbpf_match_ipv6)(nbpf_cache_t *, nbpf_buf_t *, void *, const int, const nbpf_in6_t *, nbpf_netmask_t);
	int (*nbpf_match_tcp_ports)(nbpf_cache_t *, nbpf_buf_t *, void *, const int, const uint32_t);
	int (*nbpf_match_udp_ports)(nbpf_cache_t *, nbpf_buf_t *, void *, const int, const uint32_t);
	int (*nbpf_match_icmp4)(nbpf_cache_t *, nbpf_buf_t *, void *, uint32_t);
	int (*nbpf_match_icmp6)(nbpf_cache_t *, nbpf_buf_t *, void *, uint32_t);
	int (*nbpf_match_tcpfl)(nbpf_cache_t *, nbpf_buf_t *, void *, uint32_t);
};

extern struct nbpf_ops *nbpf_ops;

static __inline int
nbops_match_ether(nbpf_buf_t *nbuf, int sd, int _res, uint16_t ethertype, uint32_t *r)
{
	return ((*nbpf_ops->nbpf_match_ether)(nbuf, sd, _res, ethertype, r));
}

static __inline int
nbops_match_proto(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, uint32_t ap)
{
	return ((*nbpf_ops->nbpf_match_proto)(npc, nbuf, nptr, ap));
}

static __inline int
nbops_match_ipv4(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, const int szsd, const nbpf_in4_t *maddr, nbpf_netmask_t mask)
{
	return ((*nbpf_ops->nbpf_match_ipv4)(npc, nbuf, nptr, szsd, maddr, mask));
}

static __inline int
nbops_match_ipv6(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, const int szsd, const nbpf_in6_t *maddr, nbpf_netmask_t mask)
{
	return ((*nbpf_ops->nbpf_match_ipv6)(npc, nbuf, nptr, szsd, maddr, mask));
}

static __inline int
nbops_match_tcp_ports(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, const int sd, const uint32_t prange)
{
	return ((*nbpf_ops->nbpf_match_tcp_ports)(npc, nbuf, nptr, sd, prange));
}

static __inline int
nbops_match_udp_ports(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, const int sd, const uint32_t prange)
{
	return ((*nbpf_ops->nbpf_match_udp_ports)(npc, nbuf, nptr, sd, prange));
}

static __inline int
nbops_match_icmp4(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, uint32_t tc)
{
	return ((*nbpf_ops->nbpf_match_icmp4)(npc, nbuf, nptr, tc));
}

static __inline int
nbops_match_icmp6(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, uint32_t tc)
{
	return ((*nbpf_ops->nbpf_match_icmp6)(npc, nbuf, nptr, tc));
}

static __inline int
nbops_match_udp_ports(nbpf_cache_t *npc, nbpf_buf_t *nbuf, void *nptr, uint32_t fl)
{
	return ((*nbpf_ops->nbpf_match_tcpfl)(npc, nbuf, nptr, fl));
}

#endif /* SYS_DEVEL_NET_NBPF_V2_NBPF_OPS_H_ */
