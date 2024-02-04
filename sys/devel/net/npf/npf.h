/*	$NetBSD: npf.h,v 1.14.2.6.4.2 2018/04/05 11:38:36 martin Exp $	*/

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
 * Public NPF interfaces.
 */

#ifndef _NPF_NET_H_
#define _NPF_NET_H_

#include <sys/param.h>
#include <sys/types.h>

#include <sys/ioctl.h>
#include <prop/proplib.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>

#define	NPF_VERSION		5

/*
 * Public declarations and definitions.
 */

/* Storage of address (both for IPv4 and IPv6) and netmask */
typedef struct in6_addr		npf_addr_t;
typedef uint8_t				npf_netmask_t;

#define	NPF_MAX_NETMASK		(128)
#define	NPF_NO_NETMASK		((npf_netmask_t)~0)

#if defined(_KERNEL)

/* Network buffer. */
typedef void				nbuf_t;

struct npf_rproc;
typedef struct npf_rproc	npf_rproc_t;

/*
 * Packet information cache.
 */
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

typedef struct npf_cache npf_cache_t;

struct npf_cache {
	/* Information flags. */
	uint32_t	npc_info;
	/* Pointers to the IP v4/v6 addresses. */
	npf_addr_t 	*npc_srcip;
	npf_addr_t 	*npc_dstip;
	/* Size (v4 or v6) of IP addresses. */
	int			npc_alen;
	uint32_t	npc_hlen;
	int			npc_proto;
	/* IPv4, IPv6. */
	union {
		struct ip			v4;
		struct ip6_hdr		v6;
	} npc_ip;
	/* TCP, UDP, ICMP. */
	union {
		struct tcphdr		tcp;
		struct udphdr		udp;
		struct icmp			icmp;
		struct icmp6_hdr	icmp6;
	} npc_l4;
};


#endif	/* _KERNEL */

#endif /* _NPF_NET_H_ */
