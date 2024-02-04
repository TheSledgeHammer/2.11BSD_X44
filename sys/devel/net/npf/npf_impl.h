/*	$NetBSD: npf_impl.h,v 1.10.2.7 2012/08/13 17:49:52 riz Exp $	*/

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
 * Private NPF structures and interfaces.
 * For internal use within NPF core only.
 */

#ifndef _NPF_IMPL_H_
#define _NPF_IMPL_H_

/* For INET/INET6 definitions. */
#include "opt_inet.h"
#include "opt_inet6.h"

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/hash.h>
#include <sys/tree.h>
#include <sys/ptree.h>
#include <sys/rwlock.h>
#include <net/if.h>

#include "npf.h"
#include "nbpf/nbpf_ncode.h"

#ifdef _NPF_DEBUG
#define	NPF_PRINTF(x)	printf x
#else
#define	NPF_PRINTF(x)
#endif

/*
 * STRUCTURE DECLARATIONS.
 */

struct npf_ruleset;
struct npf_rule;
struct npf_nat;
struct npf_session;

typedef struct npf_ruleset		npf_ruleset_t;
typedef struct npf_rule			npf_rule_t;
typedef struct npf_nat			npf_nat_t;
typedef struct npf_alg			npf_alg_t;
typedef struct npf_natpolicy	npf_natpolicy_t;
typedef struct npf_session		npf_session_t;

struct npf_sehash;
struct npf_tblent;
struct npf_table;

typedef struct npf_sehash		npf_sehash_t;
typedef struct npf_tblent		npf_tblent_t;
typedef struct npf_table		npf_table_t;

typedef npf_table_t 			*npf_tableset_t;

/*
 * DEFINITIONS.
 */

#define	NPF_DECISION_BLOCK	0
#define	NPF_DECISION_PASS	1

typedef bool (*npf_algfunc_t)(npf_cache_t *, nbuf_t *, void *);

#define	NPF_NCODE_LIMIT		1024
#define	NPF_TABLE_SLOTS		32

/*
 * SESSION STATE STRUCTURES
 */

#define	NPF_FLOW_FORW		0
#define	NPF_FLOW_BACK		1

typedef struct {
	uint32_t		nst_end;
	uint32_t		nst_maxend;
	uint32_t		nst_maxwin;
	int				nst_wscale;
} npf_tcpstate_t;

typedef struct {
	kmutex_t		nst_lock;
	int				nst_state;
	npf_tcpstate_t	nst_tcpst[2];
} npf_state_t;

#if defined(_NPF_TESTING)
void		npf_state_sample(npf_state_t *, bool);
#define	NPF_STATE_SAMPLE(n, r)	npf_state_sample(n, r)
#else
#define	NPF_STATE_SAMPLE(n, r)
#endif

/*
 * INTERFACES.
 */

/* NPF control, statistics, etc. */
void			npf_core_enter(void);
npf_ruleset_t 	*npf_core_ruleset(void);
npf_ruleset_t 	*npf_core_natset(void);
npf_tableset_t 	*npf_core_tableset(void);
void			npf_core_exit(void);
bool			npf_core_locked(void);
bool			npf_default_pass(void);
prop_dictionary_t npf_core_dict(void);


#endif /* _NPF_IMPL_H_ */
