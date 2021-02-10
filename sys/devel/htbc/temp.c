/*	$NetBSD: ext2fs_htree.h,v 1.1 2016/06/24 17:21:30 christos Exp $	*/

/*-
 * Copyright (c) 2010, 2012 Zheng Liu <lz@freebsd.org>
 * Copyright (c) 2012, Vyacheslav Matyushin
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/fs/ext2fs/htree.h 262623 2014-02-28 21:25:32Z pfg $
 */
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
/* original revision of htbc */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/lock.h>
#include <sys/lockf.h>
#include <sys/fnv_hash.h>
#include <sys/signalvar.h>
#include <sys/time.h>

#include <miscfs/specfs/specdev.h>

#include <devel/htbc/htbc.h>
#include <devel/htbc/htbc_htree.h>
#include <devel/sys/malloctypes.h>


/* null entry (on disk) */
struct htbc_hc_null {
	uint32_t	hc_type;
	int32_t		hc_len;
	uint8_t		hc_spare[0];
};

/* journal header (on-disk) */
struct htbc_hc_header {
	uint32_t	hc_type;
	int32_t		hc_len;
	uint32_t	hc_checksum;
	uint32_t	hc_generation;
	int32_t		hc_fsid[2];
	uint64_t	hc_time;
	uint32_t	hc_timensec;
	uint32_t	hc_version;

	uint32_t	hc_log_dev_bshift;
	uint32_t	hc_fs_dev_bshift;

	int64_t		hc_head;
	int64_t		hc_tail;
};

/* list of blocks (on disk) */
struct htbc_hc_blocklist {
	u_int32_t 	hc_type;
	int32_t 	hc_len;
	int32_t		hc_blkcount;
	struct {
		int64_t	hc_daddr;
		int32_t	hc_unused;
		int32_t	hc_dlen;
	} hc_blocks[0];
};

/* list of inodes (on disk) */
struct htbc_hc_inodelist {
	uint32_t	hc_type;
	int32_t		hc_len;
	int32_t		hc_inocnt;
	int32_t		hc_clear; 								/* set if previously listed inodes hould be ignored */

	struct {
		uint32_t hc_inumber;
		uint32_t hc_imode;
	} hc_inodes[0];
};

/* Holds per transaction log information */
struct htbc_entry {
	struct htbc 				*he_htbc;
	CIRCLEQ_ENTRY(htbc_entry) 	he_entries;
	size_t 						he_bufcount;			/* Count of unsynced buffers */
	size_t 						he_reclaimable_bytes;	/* Number on disk bytes for this transaction */
	int							he_error;
};

struct htbc {
	struct vnode 						*ht_devvp;			/* log on this device */
	struct mount 						*ht_mount;			/* mountpoint ht is associated with */
	struct htbc_inode					*ht_inode;			/* inode */
	CIRCLEQ_HEAD(hashchain, ht_hchain)	ht_hashchain;		/* hashchain to store transactions */

	daddr_t 							ht_logpbn;			/* Physical block number of start of log */
	int 								ht_log_dev_bshift;	/* logarithm of device block size of log device */
	int 								ht_fs_dev_bshift;	/* logarithm of device block size of filesystem device */

	size_t 								ht_circ_size; 		/* Number of bytes in buffer of log */
	size_t 								ht_circ_off;		/* Number of bytes reserved at start */

	size_t 								ht_bufcount_max;	/* Number of buffers reserved for log */
	size_t 								ht_bufbytes_max;	/* Number of buf bytes reserved for log */

	struct htbc_hc_header 				*ht_hc_header;
	void 								*ht_hc_scratch;		/* scratch space (XXX: por que?!?) */

	struct lock_object 					*ht_lock;			/* transaction lock */
	unsigned int 						ht_lock_count;		/* Count of transactions in progress */

	size_t 								ht_bufbytes;		/* Byte count of pages in ht_bufs */
	size_t 								ht_bufcount;		/* Count of buffers in ht_bufs */
	size_t 								ht_bcount;			/* Total bcount of ht_bufs */

	LIST_HEAD(, buf) 					ht_bufs; 			/* Buffers in current transaction */

	TAILQ_HEAD(, htbc_dealloc) 			ht_dealloclist;		/* list head */
	daddr_t 							*ht_deallocblks;	/* address of block */
	int 								*ht_dealloclens;	/* size of block */
	int 								ht_dealloccnt;		/* total count */
	int 								ht_dealloclim;		/* max count */

	/* hashtable of inode numbers for allocated but unlinked inodes */
	LIST_HEAD(htbc_ino_head, htbc_ino) 	*ht_inohash;
	u_long 								ht_inohashmask;
	int 								ht_inohashcnt;

	CIRCLEQ_HEAD(, htbc_entry) 			ht_entries;			/* On disk transaction accounting */

	/* hash chain */
	CIRCLEQ_ENTRY(htbc) 				ht_hchain_entries;	/* hash chain entry */
	uint32_t							ht_version;			/* hash chain version */
	uint32_t							ht_timestamp;		/* hash chain timestamp */
	uint32_t							ht_hash;			/* hash chain hash algorithm used */
};

CIRCLEQ_HEAD(htbc_hchain_head, htbc);
struct htbc_hchain {
	struct htbc_hchain_head				hc_header;		/* hash chain header */
	uint32_t							hc_version;		/* version */
	uint32_t							hc_timestamp;	/* timestamp */
	uint32_t							hc_hash;		/* hash algorithm */
};

static struct htbc_dealloc 		*htbc_dealloc;
static struct htbc_entry 		*htbc_entry;
static struct htbc_hchain 		*htbc_hchain;

struct htbc_ino {
	LIST_ENTRY(htbc_ino) 		hti_hash;
	ino_t 						hti_ino;
	mode_t 						hti_mode;
};

void
htbc_init()
{
	&htbc_entry = (struct htbc_entry *) malloc(sizeof(struct htbc_entry), M_HTBC, NULL);
	&htbc_dealloc = (struct htbc_dealloc *) malloc(sizeof(struct htbc_dealloc), M_HTBC, NULL);
	&htbc_hchain = (struct htbc_hchain *) malloc(sizeof(struct htbc_hchain), M_HTBC, NULL);
}

void
htbc_fini()
{
	FREE(&htbc_entry, M_HTBC);
	FREE(&htbc_dealloc, M_HTBC);
	FREE(&htbc_hchain, M_HTBC);
}

/****************************************************************/
/* HTBC hash chain */

void
htbc_hchain_insert_head(hc, ht, hash, timestamp, version)
	struct htbc_hchain *hc;
	struct htbc *ht;
	uint32_t hash, timestamp, version;
{
	htbc_hchain_set_hash(hc, ht, hash);
	htbc_hchain_set_timestamp(hc, ht, timestamp);
	htbc_hchain_set_version(hc, ht, version);

	CIRCLEQ_INSERT_HEAD(hc->hc_header, ht, ht_hchain_entries);
}

void
htbc_hchain_insert_tail(hc, ht, hash, timestamp, version)
	struct htbc_hchain *hc;
	struct htbc *ht;
	uint32_t hash, timestamp, version;
{
	htbc_hchain_set_hash(hc, hash);
	htbc_hchain_set_timestamp(hc, timestamp);
	htbc_hchain_set_version(hc, version);

	CIRCLEQ_INSERT_TAIL(&hc->hc_header, ht, ht_hchain_entries);
}

/* search next hchain entry by hash */
struct htbc *
htbc_hchain_search_next(hc)
	struct htbc_hchain *hc;
{
	register struct htbc *htbc;
	for (htbc = CIRCLEQ_FIRST(&hc->hc_header); htbc != NULL; htbc =	CIRCLEQ_NEXT(htbc, ht_hchain_entries)) {
		if (htbc->ht_hash == htbc_hchain_get_hash(hc)) {
			return (htbc);
		}
	}
	return (NULL);
}

/* search previous hchain entry by hash  */
struct htbc *
htbc_hchain_search_prev(hc)
	struct htbc_hchain *hc;
{
	register struct htbc *htbc;
	for (htbc = CIRCLEQ_FIRST(&hc->hc_header); htbc != NULL; htbc =	CIRCLEQ_PREV(htbc, ht_hchain_entries)) {
		if (htbc->ht_hash == htbc_hchain_get_hash(hc)) {
			return (htbc);
		}
	}
	return (NULL);
}

void
htbc_hchain_remove(hc)
	struct htbc_hchain *hc;
{
	register struct htbc *htbc;
	for (htbc = CIRCLEQ_FIRST(&hc->hc_header); htbc != NULL; htbc =	CIRCLEQ_NEXT(htbc, ht_hchain_entries)) {
		if (htbc->ht_hash == htbc_hchain_get_hash(hc)) {
			CIRCLEQ_REMOVE(&hc->hc_header, htbc, ht_hchain_entries);
		}
	}
}

static uint32_t
htbc_hchain_get_hash(hc)
	struct htbc_hchain *hc;
{
	return (hc->hc_hash);
}

static uint32_t
htbc_hchain_get_timestamp(hc)
	struct htbc_hchain *hc;
{
	return (hc->hc_timestamp);
}

static uint32_t
htbc_hchain_get_version(hc)
	struct htbc_hchain *hc;
{
	return (hc->hc_version);
}

static void
htbc_hchain_set_hash(hc, ht, hash)
	struct htbc_hchain *hc;
	struct htbc *ht;
	uint32_t hash;
{
	hc->hc_hash = hash;
	ht->ht_hash = hc->hc_hash;
}

static void
htbc_hchain_set_timestamp(hc, ht, timestamp)
	struct htbc_hchain *hc;
	struct htbc *ht;
	uint32_t timestamp;
{
	hc->hc_timestamp = timestamp;
	ht->ht_timestamp = hc->hc_timestamp;
}

static void
htbc_hchain_set_version(hc, ht, version)
	struct htbc_hchain *hc;
	struct htbc *ht;
	uint32_t version;
{
	hc->hc_version = version;
	ht->ht_version = hc->hc_version;
}

/****************************************************************/
/* HTBC Inode */

u_long
htbc_inodetrk_hash(ino, hashmask)
	ino_t ino;
	u_long hashmask;
{
    Fnv32_t hash1 = fnv_32_buf(ino, sizeof(ino), FNV1_32_INIT) % hashmask;
    Fnv32_t hash2 = (((unsigned long)ino)%hashmask);
    return (hash1^hash2);
}

static void
htbc_inodetrk_init(ht, size)
	struct htbc *ht;
	u_int size;
{
	int i;
	for (i = 0; i < size; i++) {
		LIST_INIT(ht->ht_inohash[i]);
	}
	u_long hash = htbc_inodetrk_hash();
	htbc_hchain_insert_head(NULL, ht, hash, 0, 0);
}

static void
htbc_inodetrk_free(ht, ino)
	struct htbc *ht;
	ino_t ino;
{
	register struct htbc_ino_head *hih;
	struct htbc_ino *hi;

	hih = ht->ht_inohash[htbc_inodetrk_hash(ino, ht->ht_inohashmask)];
	LIST_FOREACH(hi, hih, hti_hash) {
		if(hi->hti_ino == ino) {
			LIST_REMOVE(hi, hti_hash);
		}
	}
}

struct htbc_ino *
htbc_inodetrk_get(ht, ino)
	struct htbc *ht;
	ino_t ino;
{
	struct htbc_ino_head *hih;
	struct htbc_ino *hi;

	hih = &ht->ht_inohash[htbc_inodetrk_hash(ino, ht->ht_inohashmask)];
	LIST_FOREACH(hi, hih, hti_hash) {
		if (ino == hi->hti_ino) {
			return (hi);
		}
	}
	return (0);
}

void
htbc_register_inode(ht, ino, mode)
	struct htbc *ht;
	ino_t ino;
	mode_t mode;
{
	struct htbc_ino_head 	*hih;
	struct htbc_ino 		*hi;

	htbc_lock(ht);
	if(htbc_inodetrk_get(ht, ino) == NULL) {
		hi->hti_ino = ino;
		hi->hti_mode = mode;
		hih = &ht->ht_inohash[ino, ht->ht_inohashmask];
		LIST_INSERT_HEAD(hih, hi, hti_hash);
		ht->ht_inohashcnt++;
		htbc_unlock(ht);
	}
}

void
htbc_unregister_inode(ht, ino, mode)
	struct htbc *ht;
	ino_t ino;
	mode_t mode;
{
	struct htbc_ino *hi;
	htbc_lock(ht);
	hi = htbc_inodetrk_get(ht, ino);
	if(hi) {
		ht->ht_inohashcnt--;
		LIST_REMOVE(hi, hti_hash);
		htbc_unlock(ht);
	}
}

static __inline size_t
htbc_transaction_inodes_len(ht)
	struct htbc *ht;
{
	int blocklen = 1 << ht->ht_log_dev_bshift;
	int iph;

	/* Calculate number of inodes described in a inodelist header */
	iph = (blocklen - offsetof(struct htbc_hc_inodelist, hc_inodes)) / sizeof(((struct htbc_hc_inodelist *)0)->hc_inodes[0]);

	KASSERT(iph > 0);

	return MAX(1, howmany(ht->ht_inohashcnt, iph))*blocklen;
}

/* Calculate amount of space a transaction will take on disk */
static size_t
htbc_transaction_len(ht)
	struct htbc *ht;
{
	int blocklen = 1<<ht->ht_log_dev_bshift;
	size_t len;
	int bph;

	/* Calculate number of blocks described in a blocklist header */
	bph = (blocklen - offsetof(struct htbc_hc_blocklist, hc_blocks)) /
	    sizeof(((struct htbc_hc_blocklist *)0)->hc_blocks[0]);

	KASSERT(bph > 0);

	len = ht->ht_bcount;
	len += howmany(ht->ht_bufcount, bph)*blocklen;
	len += howmany(ht->ht_dealloccnt, bph)*blocklen;
	len += htbc_transaction_inodes_len(ht);

	return len;
}
