/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)vfs_htbc.c	1.00
 */

#include <sys/malloc.h>
#include <sys/null.h>

#include "htree.h"

#define	KDASSERT(x) 			assert(x)
#define	KASSERT(x) 				assert(x)
#define	htbc_alloc(s) 			malloc(s)
#define	htbc_free(a) 			free(a)
#define	htbc_calloc(n, s) 		calloc((n), (s))

/* locks on htbc */
#define htbc_lock(hc)			simple_lock((ht)->ht_lock)
#define htbc_unlock(hc) 		simple_unlock((ht)->ht_lock)

/* locks on hashchain */
#define hashchain_lock(hc)		simple_lock((hc)->hc_lock)
#define hashchain_unlock(hc) 	simple_unlock((hc)->hc_lock)

/* locks on hashtree */
#define hashtree_lock(hc)		simple_lock((hc)->hc_lock)
#define hashtree_unlock(hc) 	simple_unlock((hc)->hc_lock)

#define M_HCHAIN 	102
#define M_HTREE 	103

struct htbc {
	struct vnode 				*ht_devvp;			/* this device */
	struct mount 				*ht_mount;			/* mountpoint ht is associated with */
	struct hashhead				ht_hashhead;		/* contains all data pertaining to entries */

	LIST_HEAD(, buf) 			ht_bufs; 			/* Buffers in current transaction */
};

struct hashhead *blockchain;

int
htbc_start(htp, mp, vp, off, count, blksize)
	struct htbc **htp;
	struct mount *mp;
	struct vnode *vp;
	daddr_t off;
	size_t count, blksize;
{
	struct htbc *ht;
	struct vnode *devvp;
	int error;

	if (off < 0) {
		return EINVAL;
	}
	if (blksize < DEV_BSIZE) {
		return EINVAL;
	}
	if (blksize % DEV_BSIZE) {
		return EINVAL;
	}

	ht = htbc_calloc(1, sizeof(*ht));

	LIST_INIT(&ht->ht_bufs);
	CIRCLEQ_INIT(&ht->ht_hashhead);
	ht->ht_devvp = devvp;
	ht->ht_mount = mp;

	/* Initialize the blockchain header */
	{
		struct hashchain *hc;
		size_t len = 1 << ht->ht_log_dev_bshift;
		hc = htbc_calloc(1, len);
		hc->hc_length = len;
		CIRCLEQ_FIRST(&ht->ht_hashhead) = hc;
	}

	return (0);
}

/* Hashchain */

int
hashchain_hash(const char *name, int length)
{
	return (0);
}

struct hashchain *
hashchain_lookup(const char *name, int length)
{
	struct hashhead *bucket;
	register struct hashchain 	*entry;

	bucket = &blockchain[hashchain_hash(name, length)];
	hashchain_lock(entry);
	CIRCLEQ_FOREACH(entry, bucket, hc_entry) {
		if ((entry->hc_name == name) && (entry->hc_length == length)) {
			hashchain_unlock(entry);
			return (entry);
		}
	}
	hashchain_unlock(entry);
	return (NULL);
}

void
hashchain_insert(const char *name, int length)
{
	struct hashhead *bucket;
	register struct hashchain *entry;

	bucket = &blockchain[hashchain_hash(name, length)];
	entry = (struct hashchain *)malloc(sizeof(*entry), M_HCHAIN, M_WAITOK);
	entry->hc_hroot = (struct hashtree_root *)malloc(sizeof(struct hashtree_root *), M_HTREE, M_WAITOK);
	entry->hc_name = name;
	entry->hc_length = length;

	hashchain_lock(entry);
	CIRCLEQ_INSERT_HEAD(bucket, entry, hc_entry);
	//CIRCLEQ_INSERT_TAIL(bucket, entry, hc_entry);
	hashchain_unlock(entry);
	entry->hc_refcnt++;
}

void
hashchain_remove(const char *name, int length)
{
	struct hashhead *bucket;
	register struct hashchain 	*entry;

	bucket = &blockchain[hashchain_hash(name, length)];
	hashchain_lock(entry);
	CIRCLEQ_FOREACH(entry, bucket, hc_entry) {
		if ((entry->hc_name == name) && (entry->hc_length == length)) {
			CIRCLEQ_REMOVE(bucket, entry, hc_entry);
			hashchain_unlock(entry);
			entry->hc_refcnt--;
			FREE(entry, M_HCHAIN);
		}
	}
}

struct hashtree_root *
hashchain_lookup_hashtree_root(struct hashchain *chain)
{
	struct hashchain *look;
	struct hashtree_root *root;

	look = hashchain_lookup(chain->hc_name, chain->hc_length);
	if (look != NULL) {
		root = look->hc_hroot;
		return (root);
	}
	return (NULL);
}

/* Red-Black Hash Tree */

void
hashtree_setcount(struct hashtree_entry *entry, uint16_t count)
{
	entry->h_num = count;
}

void
hashtree_setlimit(struct hashtree_entry *entry, uint16_t limit)
{
	entry->h_max = limit;
}

void
hashtree_sethash(struct hashtree_entry *entry, uint32_t hash)
{
	entry->h_hash = hash;
}

void
hashtree_setblock(struct hashtree_entry *entry, uint32_t block)
{
	entry->h_block = block;
}

uint16_t
hashtree_getcount(struct hashtree_entry *entry)
{
	return (entry->h_num);
}

uint16_t
hashtree_getlimit(struct hashtree_entry *entry)
{
	return (entry->h_max);
}

uint32_t
hashtree_gethash(struct hashtree_entry *entry)
{
	return (entry->h_hash);
}

uint32_t
hashtree_getblock(struct hashtree_entry *entry)
{
	return (entry->h_block);
}

static int
hashtree_rb_hash_compare(struct htree_entry *a, struct htree_entry *b)
{
	if (a->h_hash < b->h_hash) {
		return (-1);
	} else if (a->h_hash > b->h_hash) {
		return (1);
	}
	return (0);
}

int
hashtree_rb_compare(struct htree_entry *a, struct htree_entry *b)
{
	if (a < b) {
		return (-1);
	} else if (a > b) {
		return (1);
	}
	return (hashtree_rb_hash_compare(a, b));
}

RB_PROTOTYPE(hashtree_rbtree, hashtree_entry, h_node, hashtree_rb_compare);
RB_GENERATE(hashtree_rbtree, hashtree_entry, h_node, hashtree_rb_compare);

void
hashtree_rb_insert(struct hashtree_rbtree *root, struct hashtree_entry *entry, uint32_t hash, uint32_t block)
{
	hashtree_sethash(entry, hash);
	hashtree_setblock(entry, block);
	RB_INSERT(hashtree_rbtree, root, entry);
}

void
hashtree_rb_remove(struct hashtree_rbtree *root, uint32_t hash, uint32_t block)
{
	struct hashtree_entry *entry;

	entry = hashtree_rb_lookup(root, hash, block);
	if (entry != NULL) {
		RB_REMOVE(hashtree_rbtree, root, entry);
	}
}

struct hashtree_entry *
hashtree_rb_lookup(struct hashtree_rbtree *root, uint32_t hash, uint32_t block)
{
	struct hashtree_entry *entry;
	RB_FOREACH(entry, hashtree_rbtree, root) {
		if ((entry->h_hash == hash) && (entry->h_block == block)) {
			return (entry);
		}
	}
	return (NULL);
}
