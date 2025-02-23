/*	$NetBSD: npf_conf.c,v 1.2.2.2 2013/02/11 21:49:48 riz Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
 * NPF config loading mechanism.
 *
 * There are few main operations on the config:
 * 1) Read access which is primarily from the npf_packet_handler() et al.
 * 2) Write access on particular set, mainly rule or table updates.
 * 3) Deletion of the config, which is done during the reload operation.
 *
 * Synchronisation
 *
 *	For (1) case, passive serialisation is used to allow concurrent
 *	access to the configuration set (ruleset, etc).  It guarantees
 *	that the config will not be destroyed while accessing it.
 *
 *	Writers, i.e. cases (2) and (3) use mutual exclusion and when
 *	necessary writer-side barrier of the passive serialisation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_conf.c,v 1.2.2.2 2013/02/11 21:49:48 riz Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/malloc.h>
#include <sys/pserialize.h>
#include <sys/mutex.h>

#include "npf_impl.h"

typedef struct {
	npf_ruleset_t 		*n_rules;
	npf_tableset_t 		*n_tables;
	npf_ruleset_t 		*n_nat_rules;
	npf_rprocset_t 		*n_rprocs;
	prop_dictionary_t	n_dict;
	bool				n_default_pass;
} npf_config_t;

static npf_config_t 	*npf_config			__cacheline_aligned;
static kmutex_t			npf_config_lock		__cacheline_aligned;
static pserialize_t		npf_config_psz		__cacheline_aligned;

void
npf_config_init(void)
{
	prop_dictionary_t dict;
	npf_ruleset_t *rlset, *nset;
	npf_rprocset_t *rpset;
	npf_tableset_t *tset;

	mutex_init(&npf_config_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	npf_config_psz = pserialize_create();

	/* Load the empty configuration. */
	dict = prop_dictionary_create();
	tset = npf_tableset_create();
	rpset = npf_rprocset_create();
	rlset = npf_ruleset_create(0);
	nset = npf_ruleset_create(0);
	npf_config_reload(dict, rlset, tset, nset, rpset, true);
	KASSERT(npf_config != NULL);
}

static void
npf_config_destroy(npf_config_t *nc)
{
	prop_object_release(nc->n_dict);
	npf_ruleset_destroy(nc->n_rules);
	npf_ruleset_destroy(nc->n_nat_rules);
	npf_rprocset_destroy(nc->n_rprocs);
	npf_tableset_destroy(nc->n_tables);
	npf_free(nc, M_NPF);
}

void
npf_config_fini(void)
{
	npf_config_destroy(npf_config);
	pserialize_destroy(npf_config_psz);
	mutex_destroy(&npf_config_lock);
}

/*
 * npf_config_reload: the main routine performing configuration reload.
 * Performs the necessary synchronisation and destroys the old config.
 */
void
npf_config_reload(prop_dictionary_t dict, npf_ruleset_t *rset,
    npf_tableset_t *tset, npf_ruleset_t *nset, npf_rprocset_t *rpset,
    bool flush)
{
	npf_config_t *nc, *onc;

	nc = npf_malloc(sizeof(npf_config_t), M_NPF, M_WAITOK);
	nc->n_rules = rset;
	nc->n_tables = tset;
	nc->n_nat_rules = nset;
	nc->n_rprocs = rpset;
	nc->n_dict = dict;
	nc->n_default_pass = flush;

	/*
	 * Acquire the lock and perform the first phase:
	 * - Scan and use existing dynamic tables, reload only static.
	 * - Scan and use matching NAT policies to preserve the connections.
	 */
	mutex_enter(&npf_config_lock);
	if ((onc = npf_config) != NULL) {
		npf_ruleset_reload(rset, onc->n_rules);
		npf_tableset_reload(tset, onc->n_tables);
		npf_ruleset_natreload(nset, onc->n_nat_rules);
	}

	/*
	 * Set the new config and release the lock.
	 */
	membar_sync();
	npf_config = nc;
	if (onc == NULL) {
		/* Initial load, done. */
		mutex_exit(&npf_config_lock);
		return;
	}

	/* Synchronise: drain all references. */
	pserialize_perform(npf_config_psz);
	mutex_exit(&npf_config_lock);

	/* Finally, it is safe to destroy the old config. */
	npf_config_destroy(onc);
}

/*
 * Writer-side exclusive locking.
 */

void
npf_config_enter(void)
{
	mutex_enter(&npf_config_lock);
}

void
npf_config_exit(void)
{
	mutex_exit(&npf_config_lock);
}

bool
npf_config_locked_p(void)
{
	return mutex_owned(&npf_config_lock);
}

void
npf_config_sync(void)
{
	KASSERT(npf_config_locked_p());
	pserialize_perform(npf_config_psz);
}

/*
 * Reader-side synchronisation routines.
 */

int
npf_config_read_enter(void)
{
	return pserialize_read_enter();
}

void
npf_config_read_exit(int s)
{
	pserialize_read_exit(s);
}

/*
 * Accessors.
 */

npf_ruleset_t *
npf_config_ruleset(void)
{
	return npf_config->n_rules;
}

npf_ruleset_t *
npf_config_natset(void)
{
	return npf_config->n_nat_rules;
}

npf_tableset_t *
npf_config_tableset(void)
{
	return npf_config->n_tables;
}

prop_dictionary_t
npf_config_dict(void)
{
	return npf_config->n_dict;
}

bool
npf_default_pass(void)
{
	return npf_config->n_default_pass;
}
