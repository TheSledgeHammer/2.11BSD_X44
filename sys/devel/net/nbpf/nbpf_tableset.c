/*	$NetBSD: npf_tableset.c,v 1.9.2.8 2013/02/11 21:49:48 riz Exp $	*/

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
 * NPF tableset module.
 *
 * Notes
 *
 *	The tableset is an array of tables.  After the creation, the array
 *	is immutable.  The caller is responsible to synchronise the access
 *	to the tableset.  The table can either be a hash or a tree.  Its
 *	entries are protected by a read-write lock.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_tableset.c,v 1.9.2.8 2013/02/11 21:49:48 riz Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/fnv_hash.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/rwlock.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <sys/tree.h>
#include <sys/rwlock.h>
#include <sys/mutex.h>
#include <sys/stdbool.h>
#include <net/if.h>

#include "lpm.h"
#include "ptree.h"
#include "nbpf.h"

/*
 * Table structures.
 */

struct nbpf_tblent {
	union {
		LIST_ENTRY(nbpf_tblent) 	hashq;		/* hash */
		LIST_ENTRY(nbpf_tblent) 	listent;	/* lpm */
		pt_node_t				node;
	} te_entry;
	uint16_t				te_preflen;
	int						te_alen;
	nbpf_addr_t				te_addr;
};

LIST_HEAD(nbpf_hashl, nbpf_tblent);
struct nbpf_table {
	char				t_name[16];
	/* Lock and reference count. */
	struct rwlock		t_lock;
	u_int				t_refcnt;
	/* Total number of items. */
	u_int				t_nitems;
	/* Table ID. */
	u_int				t_id;
	/* The storage type can be: a) hash b) tree. */
	int					t_type;
	struct nbpf_hashl 	*t_hashl;
	u_long				t_hashmask;
	/* Separate trees for IPv4 and IPv6. */
	pt_tree_t			t_tree[2];

	struct nbpf_hashl 	t_list;
	lpm_t 				*t_lpm;
};

#define	NBPF_ADDRLEN2TREE(alen)	((alen) >> 4)

static size_t hashsize;
static nbpf_tblent_t tblent_cache;
#define M_NBPF 	M_DEVBUF

static nbpf_tblent_t *
nbpf_tableset_malloc(size)
	size_t size;
{
	nbpf_tblent_t *ent;

	ent = (nbpf_tblent_t *)malloc(size, M_NBPF, M_NOWAIT);
	return (ent);
}

static void
nbpf_tableset_free(ent)
	nbpf_tblent_t *ent;
{
	free(ent, M_NBPF);
}

void
nbpf_tableset_init(void)
{
	nbpf_tblent_t *cache;

	cache = &tblent_cache;
	if (cache == NULL) {
		cache = nbpf_tableset_malloc(sizeof(nbpf_tblent_t));
		tblent_cache = *cache;
	}
}

void
nbpf_tableset_fini(void)
{
	nbpf_tableset_free(&tblent_cache);
}

nbpf_tableset_t *
nbpf_tableset_create(void)
{
	const size_t sz = NBPF_TABLE_SLOTS * sizeof(nbpf_table_t *);
	return (nbpf_tableset_malloc(sz));
}

void
nbpf_tableset_destroy(nbpf_tableset_t *tblset)
{
	const size_t sz = NBPF_TABLE_SLOTS * sizeof(nbpf_table_t *);
	nbpf_table_t *t;
	u_int tid;

	/*
	 * Destroy all tables (no references should be held, as ruleset
	 * should be destroyed before).
	 */
	for (tid = 0; tid < NBPF_TABLE_SLOTS; tid++) {
		t = tblset[tid];
		if (t && atomic_dec_int_nv(&t->t_refcnt) == 0) {
			nbpf_table_destroy(t);
		}
	}
	nbpf_tableset_free(tblset);
}

/*
 * npf_tableset_insert: insert the table into the specified tableset.
 *
 * => Returns 0 on success.  Fails and returns error if ID is already used.
 */
int
nbpf_tableset_insert(nbpf_tableset_t *tblset, nbpf_table_t *t)
{
	const u_int tid = t->t_id;
	int error;

	KASSERT((u_int)tid < NBPF_TABLE_SLOTS);

	if (tblset[tid] == NULL) {
		atomic_inc_int(&t->t_refcnt);
		tblset[tid] = t;
		error = 0;
	} else {
		error = EEXIST;
	}
	return (error);
}

/*
 * npf_tableset_reload: iterate all tables and if the new table is of the
 * same type and has no items, then we preserve the old one and its entries.
 *
 * => The caller is responsible for providing synchronisation.
 */
void
nbpf_tableset_reload(nbpf_tableset_t *ntset, nbpf_tableset_t *otset)
{
	for (int i = 0; i < NBPF_TABLE_SLOTS; i++) {
		nbpf_table_t *t = ntset[i], *ot = otset[i];

		if (t == NULL || ot == NULL) {
			continue;
		}
		if (t->t_nitems || t->t_type != ot->t_type) {
			continue;
		}

		/*
		 * Acquire a reference since the table has to be kept
		 * in the old tableset.
		 */
		atomic_inc_int(&ot->t_refcnt);
		ntset[i] = ot;

		/* Only reference, never been visible. */
		t->t_refcnt--;
		nbpf_table_destroy(t);
	}
}

/*
 * Few helper routines.
 */

static nbpf_tblent_t *
table_hash_lookup(const nbpf_table_t *t, const nbpf_addr_t *addr, const int alen, struct nbpf_hashl **rhtbl)
{
	uint32_t hidx;
	const struct nbpf_hashl *htbl;
	nbpf_tblent_t *ent;

	hidx = fnva_32_buf(addr, alen, FNV1_32_INIT);
	htbl = &t->t_hashl[hidx & t->t_hashmask];

	/*
	 * Lookup the hash table and check for duplicates.
	 * Note: mask is ignored for the hash storage.
	 */
	LIST_FOREACH(ent, htbl, te_entry.hashq) {
		if (ent->te_alen != alen) {
			continue;
		}
		if (memcmp(&ent->te_addr, addr, alen) == 0) {
			break;
		}
	}
	*rhtbl = htbl;
	return (ent);
}

static nbpf_tblent_t *
table_lpm_lookup(const nbpf_table_t *t, const nbpf_addr_t *addr, const int alen, struct nbpf_hashl **rlist)
{
	struct nbpf_hashl *list = &t->t_list;
	nbpf_tblent_t *ent;

	LIST_FOREACH(ent, list, te_entry.listent) {
		if ((ent->te_addr == addr) && (ent->te_alen == alen)) {
			if (lpm_lookup(t->t_lpm, addr, alen) != NULL) {
				break;
			}
			continue;
		}
		if (lpm_lookup(t->t_lpm, addr, alen) != NULL) {
			break;
		}
	}
	*rlist = list;
	return (ent);
}

static void
table_tree_destroy(pt_tree_t *tree)
{
	nbpf_tblent_t *ent;

	while ((ent = ptree_iterate(tree, NULL, PT_ASCENDING)) != NULL) {
		ptree_remove_node(tree, ent);
		nbpf_tableset_free(ent);
	}
}

static void
table_lpm_destroy(nbpf_table_t *t)
{
	nbpf_tblent_t *ent;

	while ((ent = LIST_FIRST(&t->t_list)) != NULL) {
		LIST_REMOVE(ent, te_entry.listent);
		nbpf_tableset_free(ent);
	}
	lpm_clear(t->t_lpm, NULL, NULL);
	t->t_nitems = 0;
}

/*
 * npf_table_create: create table with a specified ID.
 */
nbpf_table_t *
nbpf_table_create(u_int tid, int type, size_t hsize)
{
	nbpf_table_t *t;

	KASSERT((u_int)tid < NBPF_TABLE_SLOTS);

	t = nbpf_tableset_malloc(sizeof(nbpf_table_t));
	switch (type) {
	case NBPF_TABLE_LPM:
		t->t_lpm = lpm_create(M_NOWAIT);
		if (t->t_lpm == NULL) {
			nbpf_tableset_free(t);
			return (NULL);
		}
		LIST_INIT(&t->t_list);
		break;
	case NBPF_TABLE_TREE:
		ptree_init(&t->t_tree[0], &nbpf_table_ptree_ops,
		    (void *)(sizeof(struct in_addr) / sizeof(uint32_t)),
		    offsetof(nbpf_tblent_t, te_entry.node),
		    offsetof(nbpf_tblent_t, te_addr));
		ptree_init(&t->t_tree[1], &nbpf_table_ptree_ops,
		    (void *)(sizeof(struct in6_addr) / sizeof(uint32_t)),
		    offsetof(nbpf_tblent_t, te_entry.node),
		    offsetof(nbpf_tblent_t, te_addr));
		break;
	case NBPF_TABLE_HASH:
		t->t_hashl = hashinit(hsize, M_NBPF, &t->t_hashmask);
		hashsize = hsize;
		if (t->t_hashl == NULL) {
			nbpf_tableset_free(t);
			return NULL;
		}
		break;
	default:
		KASSERT(false);
	}
	simple_rwlock_init(&t->t_lock, "nbpf_table_rwlock");
	t->t_type = type;
	t->t_id = tid;

	return t;
}

/*
 * npf_table_destroy: free all table entries and table itself.
 */
void
nbpf_table_destroy(nbpf_table_t *t)
{
	KASSERT(t->t_refcnt == 0);

	switch (t->t_type) {
	case NBPF_TABLE_LPM:
		table_lpm_destroy(t);
		lpm_destroy(t->t_lpm);
		break;
	case NBPF_TABLE_HASH:
		for (unsigned n = 0; n <= t->t_hashmask; n++) {
			nbpf_tblent_t *ent;
			while ((ent = LIST_FIRST(&t->t_hashl[n])) != NULL) {
				LIST_REMOVE(ent, te_entry.hashq);
				nbpf_tableset_free(ent);
			}
		}
		hashfree(t->t_hashl, hashsize, M_NBPF, &t->t_hashmask);
		break;
	case NBPF_TABLE_TREE:
		table_tree_destroy(&t->t_tree[0]);
		table_tree_destroy(&t->t_tree[1]);
		break;
	default:
		KASSERT(false);
	}
	simple_rwlock_unlock(&t->t_lock);
	free(t, M_NBPF);
}

/*
 * npf_table_check: validate ID and type.
 */
int
nbpf_table_check(const nbpf_tableset_t *tset, u_int tid, int type)
{

	if ((u_int)tid >= NBPF_TABLE_SLOTS) {
		return EINVAL;
	}
	if (tset[tid] != NULL) {
		return EEXIST;
	}
	switch (type) {
	case NBPF_TABLE_LPM:
	case NBPF_TABLE_HASH:
	case NBPF_TABLE_TREE:
	default:
		return EINVAL;
	}
	return 0;
}

static int
table_cidr_check(const u_int aidx, const nbpf_addr_t *addr, const nbpf_netmask_t mask)
{

	if (mask > NBPF_MAX_NETMASK && mask != NBPF_NO_NETMASK) {
		return EINVAL;
	}
	if (aidx > 1) {
		return EINVAL;
	}

	/*
	 * For IPv4 (aidx = 0) - 32 and for IPv6 (aidx = 1) - 128.
	 * If it is a host - shall use NPF_NO_NETMASK.
	 */
	if (mask >= (aidx ? 128 : 32) && mask != NBPF_NO_NETMASK) {
		return EINVAL;
	}
	return 0;
}

/*
 * npf_table_insert: add an IP CIDR entry into the table.
 */
int
nbpf_table_insert(nbpf_tableset_t *tset, u_int tid, const int alen, const nbpf_addr_t *addr, const nbpf_netmask_t mask)
{
	const u_int aidx = NBPF_ADDRLEN2TREE(alen);
	nbpf_tblent_t *ent;
	nbpf_table_t *t;
	int error;

	if ((u_int)tid >= NBPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	error = table_cidr_check(aidx, addr, mask);
	if (error) {
		return error;
	}
	ent = nbpf_tableset_malloc(sizeof(&tblent_cache));
	memcpy(&ent->te_addr, addr, alen);
	ent->te_alen = alen;

	/*
	 * Insert the entry.  Return an error on duplicate.
	 */
	simple_rwlock_lock(&t->t_lock, RW_WRITER);
	switch (t->t_type) {
	case NBPF_TABLE_LPM: {
		const unsigned preflen = (mask == NBPF_NO_NETMASK) ? (alen * 8) : mask;
		ent->te_preflen = preflen;

		if (lpm_lookup(t->t_lpm, addr, alen) == NULL
				&& lpm_insert(t->t_lpm, addr, alen, preflen, ent) == 0) {
			LIST_INSERT_HEAD(&t->t_list, ent, te_entry.listent);
			t->t_nitems++;
			error = 0;
		} else {
			error = EEXIST;
		}
		break;
	}
	case NBPF_TABLE_HASH: {
		struct nbpf_hashl *htbl;

		/*
		 * Hash tables by the concept support only IPs.
		 */
		if (mask != NBPF_NO_NETMASK) {
			error = EINVAL;
			break;
		}
		if (!table_hash_lookup(t, addr, alen, &htbl)) {
			LIST_INSERT_HEAD(htbl, ent, te_entry.hashq);
			t->t_nitems++;
		} else {
			error = EEXIST;
		}
		break;
	}
	case NBPF_TABLE_TREE: {
		pt_tree_t *tree = &t->t_tree[aidx];
		bool ok;

		/*
		 * If no mask specified, use maximum mask.
		 */
		ok = (mask != NBPF_NO_NETMASK) ?
		    ptree_insert_mask_node(tree, ent, mask) :
		    ptree_insert_node(tree, ent);
		if (ok) {
			t->t_nitems++;
			error = 0;
		} else {
			error = EEXIST;
		}
		break;
	}
	default:
		KASSERT(false);
	}
	simple_rwlock_unlock(&t->t_lock);

	if (error) {
		nbpf_tableset_free(ent);
	}
	return error;
}

/*
 * npf_table_remove: remove the IP CIDR entry from the table.
 */
int
nbpf_table_remove(nbpf_tableset_t *tset, u_int tid, const int alen, const nbpf_addr_t *addr, const nbpf_netmask_t mask)
{
	const u_int aidx = NBPF_ADDRLEN2TREE(alen);
	nbpf_tblent_t *ent;
	nbpf_table_t *t;
	int error;

	error = table_cidr_check(aidx, addr, mask);
	if (error) {
		return error;
	}

	if ((u_int)tid >= NBPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	simple_rwlock_lock(&t->t_lock);
	switch (t->t_type) {
	case NBPF_TABLE_LPM:
		ent = lpm_lookup(t->t_lpm, addr, alen);
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_entry.listent);
			lpm_remove(t->t_lpm, &ent->te_addr, ent->te_alen, ent->te_preflen);
			t->t_nitems--;
		} else {
			error = ENOENT;
		}
		break;
	case NBPF_TABLE_HASH: {
		struct nbpf_hashl *htbl;

		ent = table_hash_lookup(t, addr, alen, &htbl);
		if (__predict_true(ent != NULL)) {
			LIST_REMOVE(ent, te_entry.hashq);
			t->t_nitems--;
		}
		break;
	}
	case NBPF_TABLE_TREE: {
		pt_tree_t *tree = &t->t_tree[aidx];

		ent = ptree_find_node(tree, addr);
		if (__predict_true(ent != NULL)) {
			ptree_remove_node(tree, ent);
			t->t_nitems--;
		}
		break;
	}
	default:
		KASSERT(false);
		ent = NULL;
	}
	simple_rwlock_unlock(&t->t_lock);

	if (ent == NULL) {
		return ENOENT;
	}
	nbpf_tableset_free(ent);
	return 0;
}

/*
 * npf_table_lookup: find the table according to ID, lookup and match
 * the contents with the specified IP address.
 */
int
nbpf_table_lookup(nbpf_tableset_t *tset, u_int tid, const int alen, const nbpf_addr_t *addr)
{
	const u_int aidx = NBPF_ADDRLEN2TREE(alen);
	nbpf_tblent_t *ent;
	nbpf_table_t *t;

	if (__predict_false(aidx > 1)) {
		return EINVAL;
	}

	if ((u_int)tid >= NBPF_TABLE_SLOTS || (t = tset[tid]) == NULL) {
		return EINVAL;
	}

	simple_rwlock_lock(&t->t_lock);
	switch (t->t_type) {
	case NBPF_TABLE_LPM: {
		struct nbpf_hashl *list;
		ent = table_lpm_lookup(t, addr, alen, &list);
		break;
	}
	case NBPF_TABLE_HASH: {
		struct nbpf_hashl *htbl;
		ent = table_hash_lookup(t, addr, alen, &htbl);
		break;
	}
	case NBPF_TABLE_TREE: {
		ent = ptree_find_node(&t->t_tree[aidx], addr);
		break;
	}
	default:
		KASSERT(false);
		ent = NULL;
	}
	simple_rwlock_unlock(&t->t_lock);

	return ent ? 0 : ENOENT;
}

int
nbpf_mktable(tset, tbl, tid, type, hsize)
	nbpf_tableset_t **tset;
	nbpf_table_t **tbl;
	u_int tid;
	int type;
	size_t hsize;
{
    nbpf_tableset_t *ts;
    nbpf_table_t    *tb;
	int error;

	ts = nbpf_tableset_create();
	if (ts == NULL) {
		return (ENOMEM);
	}
	error = nbpf_table_check(ts, tid, type);
	if (error != 0) {
		return (error);
	}
	tb = nbpf_table_create(tid, type, hsize);
	if (tb == NULL) {
		return (ENOMEM);
	}
	error = nbpf_tableset_insert(ts, tb);
	if (error != 0) {
		return (error);
	}
    *tset = ts;
    *tbl = tb
	return (0);
}

int
nbpf_table_ioctl(nbiot, tblset)
	struct nbpf_ioctl_table *nbiot;
	nbpf_tableset_t *tblset;
{
	int error;

	switch (nbiot->nct_cmd) {
	case NBPF_CMD_TABLE_LOOKUP:
		error = nbpf_table_lookup(tblset, nbiot->nct_tid, nbiot->nct_alen, &nbiot->nct_addr);
		break;
	case NBPF_CMD_TABLE_ADD:
		error = nbpf_table_insert(tblset, nbiot->nct_tid, nbiot->nct_alen, &nbiot->nct_addr, nbiot->nct_mask);
		break;
	case NBPF_CMD_TABLE_REMOVE:
		error = nbpf_table_remove(tblset, nbiot->nct_tid, nbiot->nct_alen, &nbiot->nct_addr, nbiot->nct_mask);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}
