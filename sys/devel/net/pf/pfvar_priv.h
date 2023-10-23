/*	$NetBSD: pfvar.h,v 1.8 2004/12/04 14:21:23 peter Exp $	*/
/*	$OpenBSD: pfvar.h,v 1.202 2004/07/12 00:50:22 itojun Exp $ */

/*
 * Copyright (c) 2001 Daniel Hartmeier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _NET_PF_PFVAR_PRIV_H_
#define _NET_PF_PFVAR_PRIV_H_

#include <sys/queue.h>
#include <sys/tree.h>

#ifdef notyet
struct pf_state_key {
	struct pf_state_host 	lan;
	struct pf_state_host 	gwy;
	struct pf_state_host 	ext;
	sa_family_t	 			af;
	u_int8_t	 			proto;
	u_int8_t	 			direction;
	u_int8_t	 			pad;

	RB_ENTRY(pf_state_key)	entry_lan_ext;
	RB_ENTRY(pf_state_key)	entry_ext_gwy;
	struct pf_statelist	 	states;
	u_short		 			refcnt;	/* same size as if_index */
};

struct pf_state {
	u_int64_t		 		id;
	u_int32_t		 		creatorid;
	u_int32_t		 		pad;

	TAILQ_ENTRY(pf_state)	entry_list;
	TAILQ_ENTRY(pf_state)	next;
	RB_ENTRY(pf_state)	 	entry_id;
	struct pf_state_peer	src;
	struct pf_state_peer	dst;
	union pf_rule_ptr	 	rule;
	union pf_rule_ptr	 	anchor;
	union pf_rule_ptr	 	nat_rule;
	struct pf_addr		 	rt_addr;
	struct pf_state_key		*state_key;
	struct pfi_kif			*kif;
	struct pfi_kif			*rt_kif;
	struct pf_src_node		*src_node;
	struct pf_src_node		*nat_src_node;
	u_int64_t		 		packets[2];
	u_int64_t		 		bytes[2];
	u_int32_t		 		creation;
	u_int32_t		 		expire;
	u_int32_t		 		pfsync_time;
	u_int16_t		 		tag;
	u_int8_t		 		log;
	u_int8_t		 		allow_opts;
	u_int8_t		 		timeout;
	u_int8_t		 		sync_flags;
#define	PFSTATE_NOSYNC	 	0x01
#define	PFSTATE_FROMSYNC 	0x02
#define	PFSTATE_STALE	 	0x04
};
#endif

TAILQ_HEAD(pf_statelist, pf_state);

struct pf_state_key {
	struct pf_state_host 	lan;
	struct pf_state_host 	gwy;
	struct pf_state_host 	ext;
	sa_family_t	 			af;
	u_int8_t	 			proto;
	u_int8_t	 			direction;
	u_int8_t	 			pad;

	struct pf_statelist	 	states;
	u_short		 			refcnt;	/* same size as if_index */
};

struct pf_state {
	u_int64_t	 			id;
	union {
		struct {
			RB_ENTRY(pf_state)	 	entry_lan_ext;
			RB_ENTRY(pf_state)	 	entry_ext_gwy;
			RB_ENTRY(pf_state)	 	entry_id;
			TAILQ_ENTRY(pf_state)	entry_updates;
			TAILQ_ENTRY(pf_state)	next;
			struct pfi_kif			*kif;
		} s;
		char			 	ifname[IFNAMSIZ];
	} u;
	struct pf_state_host 	lan;
	struct pf_state_host 	gwy;
	struct pf_state_host 	ext;
	struct pf_state_peer 	src;
	struct pf_state_peer 	dst;
	union pf_rule_ptr 		rule;
	union pf_rule_ptr 		anchor;
	union pf_rule_ptr 		nat_rule;
	struct pf_addr	 		rt_addr;
	struct pf_state_key		*state_key;
	struct pfi_kif			*rt_kif;
	struct pf_src_node		*src_node;
	struct pf_src_node		*nat_src_node;
	u_int32_t	 			creation;
	u_int32_t	 			expire;
	u_int32_t	 			pfsync_time;
	u_int32_t	 			packets[2];
	u_int32_t	 			bytes[2];
	u_int32_t	 			creatorid;
	/*
	sa_family_t	 			af;
	u_int8_t	 			proto;
	u_int8_t	 			direction;
	*/
	u_int16_t	 			tag;
	u_int8_t	 			log;
	u_int8_t	 			allow_opts;
	u_int8_t	 			timeout;
	u_int8_t	 			sync_flags;
#define	PFSTATE_NOSYNC	 	0x01
#define	PFSTATE_FROMSYNC 	0x02
#define	PFSTATE_STALE		0x04
	//u_int8_t	 			pad;
};
#endif /* _NET_PF_PFVAR_PRIV_H_ */
