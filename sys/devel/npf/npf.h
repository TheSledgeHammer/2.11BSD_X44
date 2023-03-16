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

/*
 * Public NPF interfaces.
 */

#ifndef _NPF_NET_H_
#define _NPF_NET_H_

#include <sys/param.h>
#include <sys/types.h>

#include <sys/ioctl.h>

#include <devel/sys/properties.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>

#define	NPF_VERSION			9

/*
 * Public declarations and definitions.
 */

/* Storage of address (both for IPv4 and IPv6) and netmask */
typedef struct in6_addr		npf_addr_t;
typedef uint8_t				npf_netmask_t;

#define	NPF_MAX_NETMASK		(128)
#define	NPF_NO_NETMASK		((npf_netmask_t)~0)

//#if defined(_KERNEL)
#define	NPF_DECISION_BLOCK	0
#define	NPF_DECISION_PASS	1

#define	NPF_EXT_MODULE(name, req)	\
    MODULE(MODULE_CLASS_MISC, name, "npf," req)

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

#define	NPC_IP4			0x01	/* Indicates fetched IPv4 header. */
#define	NPC_IP6			0x02	/* Indicates IPv6 header. */
#define	NPC_IPFRAG		0x04	/* IPv4/IPv6 fragment. */
#define	NPC_LAYER4		0x08	/* Layer 4 has been fetched. */

#define	NPC_TCP			0x10	/* TCP header. */
#define	NPC_UDP			0x20	/* UDP header. */
#define	NPC_ICMP		0x40	/* ICMP header. */
#define	NPC_ICMP_ID		0x80	/* ICMP with query ID. */

#define	NPC_ALG_EXEC	0x100	/* ALG execution. */

#define	NPC_IP46	(NPC_IP4|NPC_IP6)

typedef struct {
	/* Information flags. */
	uint32_t				npc_info;
	/* Pointers to the IP v4/v6 addresses. */
	npf_addr_t 				*npc_srcip;
	npf_addr_t 				*npc_dstip;
	/* Size (v4 or v6) of IP addresses. */
	uint8_t					npc_alen;
	uint8_t					npc_hlen;
	uint16_t				npc_proto;
	/* IPv4, IPv6. */
	union {
		struct ip 			*v4;
		struct ip6_hdr 		*v6;
	} npc_ip;
	/* TCP, UDP, ICMP. */
	union {
		struct tcphdr 		*tcp;
		struct udphdr 		*udp;
		struct icmp 		*icmp;
		struct icmp6_hdr 	*icmp6;
		void 				*hdr;
	} npc_l4;
} npf_cache_t;

#endif /* _NPF_NET_H_ */
