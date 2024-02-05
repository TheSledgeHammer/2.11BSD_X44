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
#include <sys/types.h>
#include <sys/stdbool.h>

#include <sys/ioctl.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>

typedef void 				nbpf_buf_t;
typedef void 				nbpf_cache_t;
typedef struct nbpf_state	nbpf_state_t;

union nbpf_ipv4;
union nbpf_ipv6;
union nbpf_port;
union nbpf_icmp;

struct nbpf_state {
	/* Information flags. */
	uint32_t 			nbs_info;
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

/* Network buffer interface. */
void 	*nbpf_dataptr(void *);
void 	*nbpf_advance(nbpf_buf_t **, void *, u_int);
int		nbpf_advfetch(nbpf_buf_t **, void **, u_int, size_t, void *);
int		nbpf_advstore(nbpf_buf_t **, void **, u_int, size_t, void *);
int		nbpf_fetch_datum(nbpf_buf_t *, void *, size_t, void *);
int		nbpf_store_datum(nbpf_buf_t *, void *, size_t, void *);
void	nbpf_cksum_barrier(nbpf_buf_t *);
int		nbpf_add_tag(nbpf_buf_t *, uint32_t, uint32_t);
int		nbpf_find_tag(nbpf_buf_t *, uint32_t, void **);

#endif /* _NET_NBPF_H_ */
