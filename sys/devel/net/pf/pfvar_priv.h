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

/* keep synced with struct pf_state_key, used in RB_FIND */
struct pf_state_key_cmp {
	struct pf_state_host lan;
	struct pf_state_host gwy;
	struct pf_state_host ext;
	sa_family_t	 af;
	u_int8_t	 proto;
	u_int8_t	 direction;
	u_int8_t	 pad;
};

TAILQ_HEAD(pf_statelist, pf_state);

struct pf_state_key {
	struct pf_state_host lan;
	struct pf_state_host gwy;
	struct pf_state_host ext;
	sa_family_t	 af;
	u_int8_t	 proto;
	u_int8_t	 direction;
	u_int8_t	 pad;

	RB_ENTRY(pf_state_key)	entry_lan_ext;
	RB_ENTRY(pf_state_key)	entry_ext_gwy;
	struct pf_statelist	 	states;
	u_short		 refcnt;	/* same size as if_index */
};

/* keep synced with struct pf_state, used in RB_FIND */
struct pf_state_cmp {
	u_int64_t	 id;
	u_int32_t	 creatorid;
	u_int32_t	 pad;
};

#endif /* _NET_PF_PFVAR_PRIV_H_ */
