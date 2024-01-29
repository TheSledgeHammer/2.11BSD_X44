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

#ifndef _NPF_NBPF_NET_H_
#define _NPF_NBPF_NET_H_

/*
 * NPF compatibility with NBPF
 */

#include <devel/net/nbpf/nbpf.h>

#define	NPC_IP4				NBPC_IP4		/* Indicates fetched IPv4 header. */
#define	NPC_IP6				NBPC_IP6		/* Indicates IPv6 header. */
#define	NPC_IPFRAG			NBPC_IPFRAG		/* IPv4/IPv6 fragment. */
#define	NPC_LAYER4			NBPC_LAYER4		/* Layer 4 has been fetched. */

#define	NPC_TCP				NBPC_TCP		/* TCP header. */
#define	NPC_UDP				NBPC_UDP		/* UDP header. */
#define	NPC_ICMP			NBPC_ICMP		/* ICMP header. */
#define	NPC_ICMP_ID			0x80			/* ICMP with query ID. */

#define	NPC_ALG_EXEC		0x100			/* ALG execution. */

#define	NPC_IP46			NBPC_IP46

typedef struct nbpf_cache	npf_cache_t;

static inline bool
npf_iscached(const npf_cache_t *npc, const int inf)
{
	return (nbpf_iscached(npc, inf));
}

#endif /* _NPF_NBPF_NET_H_ */
