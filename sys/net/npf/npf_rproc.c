/*	$NetBSD: npf_rproc.c,v 1.1.2.4 2013/02/11 21:49:48 riz Exp $	*/

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
 * NPF extension and rule procedure interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/malloc.h>
#include <sys/mutex.h>

#include "npf_impl.h"

#define	EXT_NAME_LEN		32

typedef struct npf_ext {
	char				ext_callname[EXT_NAME_LEN];
	LIST_ENTRY(npf_ext)	ext_entry;
	const npf_ext_ops_t *ext_ops;
	unsigned			ext_refcnt;
} npf_ext_t;

struct npf_rprocset {
	LIST_HEAD(, npf_rproc)	rps_list;
};

#define	RPROC_NAME_LEN		32
#define	RPROC_EXT_COUNT		16

struct npf_rproc {
	/* Flags and reference count. */
	uint32_t		rp_flags;
	u_int			rp_refcnt;

	/* Associated extensions and their metadata . */
	unsigned		rp_ext_count;
	npf_ext_t *		rp_ext[RPROC_EXT_COUNT];
	void *			rp_ext_meta[RPROC_EXT_COUNT];

	/* Name of the procedure and list entry. */
	char					rp_name[RPROC_NAME_LEN];
	LIST_ENTRY(npf_rproc)	rp_entry;
};

static LIST_HEAD(, npf_ext)	ext_list	__cacheline_aligned;
static kmutex_t			ext_lock	__cacheline_aligned;

#define npf_ext_malloc(size)	(npf_malloc(size, M_NPF_EXT, M_NOWAIT))
#define npf_ext_free(addr)		(npf_free(addr, M_NPF_EXT))

void
npf_ext_sysinit(void)
{
	mutex_init(&ext_lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&ext_list);
}

void
npf_ext_sysfini(void)
{
	KASSERT(LIST_EMPTY(&ext_list));
	mutex_destroy(&ext_lock);
}

/*
 * NPF extension management for the rule procedures.
 */

static npf_ext_t *
npf_ext_lookup(const char *name)
{
	npf_ext_t *ext = NULL;

	KASSERT(mutex_owned(&ext_lock));

	LIST_FOREACH(ext, &ext_list, ext_entry)
		if (strcmp(ext->ext_callname, name) == 0)
			break;
	return ext;
}

void *
npf_ext_register(const char *name, const npf_ext_ops_t *ops)
{
	npf_ext_t *ext;

	ext = npf_ext_malloc(sizeof(npf_ext_t));
	strlcpy(ext->ext_callname, name, EXT_NAME_LEN);
	ext->ext_ops = ops;

	mutex_enter(&ext_lock);
	if (npf_ext_lookup(name)) {
		mutex_exit(&ext_lock);
		npf_ext_free(ext);
		return NULL;
	}
	LIST_INSERT_HEAD(&ext_list, ext, ext_entry);
	mutex_exit(&ext_lock);

	return (void *)ext;
}

int
npf_ext_unregister(void *extid)
{
	npf_ext_t *ext = extid;

	/*
	 * Check if in-use first (re-check with the lock held).
	 */
	if (ext->ext_refcnt) {
		return EBUSY;
	}

	mutex_enter(&ext_lock);
	if (ext->ext_refcnt) {
		mutex_exit(&ext_lock);
		return EBUSY;
	}
	KASSERT(npf_ext_lookup(ext->ext_callname));
	LIST_REMOVE(ext, ext_entry);
	mutex_exit(&ext_lock);

	npf_ext_free(ext);
	return 0;
}

int
npf_ext_construct(const char *name, npf_rproc_t *rp, prop_dictionary_t params)
{
	const npf_ext_ops_t *extops;
	npf_ext_t *ext;
	unsigned i;
	int error;

	if (rp->rp_ext_count >= RPROC_EXT_COUNT) {
		return ENOSPC;
	}

	mutex_enter(&ext_lock);
	ext = npf_ext_lookup(name);
	if (ext) {
		atomic_inc_uint(&ext->ext_refcnt);
	}
	mutex_exit(&ext_lock);

	if (!ext) {
		return ENOENT;
	}

	extops = ext->ext_ops;
	KASSERT(extops != NULL);

	error = extops->ctor(rp, params);
	if (error) {
		atomic_dec_uint(&ext->ext_refcnt);
		return error;
	}
	i = rp->rp_ext_count++;
	rp->rp_ext[i] = ext;
	return 0;
}

/*
 * Rule procedure management.
 */

npf_rprocset_t *
npf_rprocset_create(void)
{
	npf_rprocset_t *rpset;

	rpset = npf_ext_malloc(sizeof(npf_rprocset_t));
	LIST_INIT(&rpset->rps_list);
	return rpset;
}

void
npf_rprocset_destroy(npf_rprocset_t *rpset)
{
	npf_rproc_t *rp;

	while ((rp = LIST_FIRST(&rpset->rps_list)) != NULL) {
		LIST_REMOVE(rp, rp_entry);
		npf_rproc_release(rp);
	}
	npf_ext_free(rpset);
}

/*
 * npf_rproc_lookup: find a rule procedure by the name.
 */
npf_rproc_t *
npf_rprocset_lookup(npf_rprocset_t *rpset, const char *name)
{
	npf_rproc_t *rp;

	LIST_FOREACH(rp, &rpset->rps_list, rp_entry) {
		if (strncmp(rp->rp_name, name, RPROC_NAME_LEN) == 0)
			break;
	}
	return rp;
}

/*
 * npf_rproc_insert: insert a new rule procedure into the set.
 */
void
npf_rprocset_insert(npf_rprocset_t *rpset, npf_rproc_t *rp)
{
	LIST_INSERT_HEAD(&rpset->rps_list, rp, rp_entry);
}

/*
 * npf_rproc_create: construct a new rule procedure, lookup and associate
 * the extension calls with it.
 */
npf_rproc_t *
npf_rproc_create(prop_dictionary_t rpdict)
{
	const char *name;
	npf_rproc_t *rp;

	if (!prop_dictionary_get_cstring_nocopy(rpdict, "name", &name)) {
		return NULL;
	}

	rp = npf_ext_malloc(sizeof(npf_rproc_t));
	rp->rp_refcnt = 1;

	strlcpy(rp->rp_name, name, RPROC_NAME_LEN);
	prop_dictionary_get_uint32(rpdict, "flags", &rp->rp_flags);
	return rp;
}

/*
 * npf_rproc_acquire: acquire the reference on the rule procedure.
 */
void
npf_rproc_acquire(npf_rproc_t *rp)
{
	atomic_inc_uint(&rp->rp_refcnt);
}

/*
 * npf_rproc_release: drop the reference count and destroy the rule
 * procedure on the last reference.
 */
void
npf_rproc_release(npf_rproc_t *rp)
{

	KASSERT(rp->rp_refcnt > 0);
	if (atomic_dec_uint_nv(&rp->rp_refcnt) != 0) {
		return;
	}
	/* XXXintr */
	for (unsigned i = 0; i < rp->rp_ext_count; i++) {
		npf_ext_t *ext = rp->rp_ext[i];
		const npf_ext_ops_t *extops = ext->ext_ops;

		extops->dtor(rp, rp->rp_ext_meta[i]);
		atomic_dec_uint(&ext->ext_refcnt);
	}
	npf_ext_free(rp);
}

void
npf_rproc_assign(npf_rproc_t *rp, void *params)
{
	unsigned i = rp->rp_ext_count;

	/* Note: params may be NULL. */
	KASSERT(i < RPROC_EXT_COUNT);
	rp->rp_ext_meta[i] = params;
}

/*
 * npf_rproc_run: run the rule procedure by executing each extension call.
 *
 * => Reference on the rule procedure must be held.
 */
void
npf_rproc_run(npf_cache_t *npc, nbuf_t *nbuf, npf_rproc_t *rp, int *decision)
{
	const unsigned extcount = rp->rp_ext_count;

	KASSERT(!nbuf_flag_p(nbuf, NBUF_DATAREF_RESET));
	KASSERT(rp->rp_refcnt > 0);

	for (unsigned i = 0; i < extcount; i++) {
		const npf_ext_t *ext = rp->rp_ext[i];
		const npf_ext_ops_t *extops = ext->ext_ops;

		KASSERT(ext->ext_refcnt > 0);
		extops->proc(npc, nbuf, rp->rp_ext_meta[i], decision);

		if (nbuf_flag_p(nbuf, NBUF_DATAREF_RESET)) {
			npf_recache(npc, nbuf);
		}
	}
}
