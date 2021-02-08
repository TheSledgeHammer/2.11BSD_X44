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
 */

/* A structure revision of the blockchain */
#include <sys/time.h>
#include <sys/user.h>
#include "blockchain.h"
#include "htbc_htree.h"
#include "sys/malloctypes.h"

struct hashchain 				*blockchain;				/* add to htbc structure */

#define HASH_VERSION			HTREE_HALF_MD4				/* make configurable */
#define HASH_SEED(hash_seed) 	(random_hash_seed(hash_seed));
#define HASH_MAJOR 				(prospector32(random()))
#define HASH_MINOR 				(prospector32(random()))


struct htbc_dealloc {
	TAILQ_ENTRY(htbc_dealloc) 			hd_entries;
	daddr_t 							hd_blkno;			/* address of block */
	int 								hd_len;				/* size of block */
};

struct htbc2 {
	struct vnode 						*ht_devvp;			/* this device */

	struct mount 						*ht_mount;			/* mountpoint ht is associated with */
	CIRCLEQ_HEAD(hashchain, ht_hchain)	ht_hashchain;		/* hashchain to store transactions */

	struct htbc_inode					*ht_inode;			/* inode */



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
};

static struct htbc_dealloc 		*htbc_dealloc;

htbc_init()
{
	&htbc_dealloc = (struct htbc_dealloc *) malloc(sizeof(struct htbc_dealloc), M_HTBC, NULL);
}

int
htbc_start(htp, mp, vp, off, count, blksize)
	struct htbc **htp;
	struct mount *mp;
	struct vnode *vp;
	daddr_t off;
	size_t count, blksize;
{
	struct htbc *ht;
	struct htbc_inode *htip;
	struct vnode *devvp;

	LIST_INIT(&ht->ht_bufs);
	CIRCLEQ_INIT(&ht->ht_hashchain);

	htip = ht->ht_inode;
	htip->hi_vnode = vp;
	ht->ht_devvp = htip->hi_devvp = devvp;
	ht->ht_mount = mp;

	htbc_chain_lookup2(name, len);

	return (0);
}

int
htbc_doflush(ht, lockcount)
	struct htbc2 *ht;
	unsigned int lockcount;
{
	size_t trans_len = htbc_transaction_len(ht, ht->ht_bcount, ht->ht_bufcount, ht->ht_dealloccnt);

	int doflush = ((ht->ht_bufbytes + (lockcount * MAXPHYS)) > ht->ht_bufbytes_max / 2) ||
			((ht->ht_bufcount + (lockcount * 10)) > ht->ht_bufcount_max / 2) ||
			(htbc_transaction_len(ht) > ht->ht_circ_size / 2) ||
			(ht->ht_dealloccnt >= (ht->ht_dealloclim / 2));

	return (doflush);
}

/****************************************************************/
/* HTBC hash chain */

void
random_hash_seed(hash_seed)
	uint32_t* hash_seed;
{
	u_long rand;

	for(int i = 0; i < 4; i++) {
		rand = prospector32(random());
		hash_seed[i] = rand;
	}
}

int
htbc_chain_hash(const char *name, int len)
{
	register uint32_t *hash_seed;

	HASH_SEED(hash_seed);
	hash = htbc_htree_hash(name, len, hash_seed, HASH_VERSION, HASH_MAJOR, HASH_MINOR);
	return (hash);
}

static void
get_hash_seed(struct htbc_inode *hi, uint32_t *seed)
{
	for (int i = 0; i < 4; i++) {
		seed[i] = hi->hi_hash_seed[i];
	}
}

/* HTBC Hashchain/Blockchain v1 */
void
htbc_chain_insert(chain, name, len)
	struct ht_hchain 	*chain;
	const char 			*name;
	int 				len;
{
	register struct hashchain *bucket;

	chain->hc_name = name;
	chain->hc_len = len;

	bucket = &blockchain[htbc_chain_hash(name, len)];
	chain->hc_trans = (struct ht_htransaction *)malloc(sizeof(struct ht_htransaction *), M_HTREE, M_WAITOK);
	chain->hc_hroot = (struct htree_root *)malloc(sizeof(struct htree_root), M_HTREE, M_WAITOK);
	hashchain_lock(chain);
	htbc_add_transaction(chain);
	CIRCLEQ_INSERT_HEAD(bucket, chain, hc_entry);
	hashchain_unlock(chain);
	chain->hc_refcnt++;
}

struct ht_hchain *
htbc_chain_lookup(name, len)
	const char 			*name;
	int 				len;
{
	register struct hashchain 	*bucket;
	struct ht_hchain 			*chain;

	bucket = &blockchain[htbc_chain_hash(name, len)];
	hashchain_lock(chain);
	for(chain = CIRCLEQ_FIRST(bucket); chain != NULL; chain = CIRCLEQ_NEXT(chain, hc_entry)) {
		if((chain->hc_name == name) && (chain->hc_len == len)) {
			hashchain_unlock(chain);
			return (chain);
		}
	}
	hashchain_unlock(chain);
	return (NULL);
}

void
htbc_chain_remove(chain)
	struct ht_hchain *chain;
{
	register struct hashchain 	*bucket;

	bucket = &blockchain[htbc_chain_hash(chain->hc_name, chain->hc_len)];
	hashchain_lock(chain);
	CIRCLEQ_REMOVE(bucket, chain, hc_entry);
	htbc_remove_transaction(chain);
	hashchain_unlock(chain);
	chain->hc_refcnt--;
}

/* HTBC Hashchain/Blockchain v2 */
void
htbc_chain_insert2(name, len)
	const char 			*name;
	int 				len;
{
	struct hashchain *bucket;
	register struct ht_hchain *entry;

	bucket = &blockchain[htbc_chain_hash(name, len)];
	entry = (struct ht_hchain *)malloc(sizeof(*entry), M_HTREE, M_WAITOK);
	entry->hc_trans = (struct ht_htransaction *)malloc(sizeof(struct ht_htransaction *), M_HTREE, M_WAITOK);
	entry->hc_hroot = (struct htree_root *)malloc(sizeof(struct htree_root *), M_HTREE, M_WAITOK);
	entry->hc_name = name;
	entry->hc_len = len;
	htbc_add_transaction(entry);

	hashchain_lock(entry);
	CIRCLEQ_INSERT_HEAD(bucket, entry, hc_entry);
	hashchain_unlock(entry);
	entry->hc_refcnt++;
}

struct ht_hchain *
htbc_chain_lookup2(name, len)
	const char 			*name;
	int 				len;
{
	struct hashchain *bucket;
	register struct ht_hchain *entry;

	bucket = &blockchain[htbc_chain_hash(name, len)];
	hashchain_lock(entry);
	for(entry = CIRCLEQ_FIRST(bucket); entry != NULL; entry = CIRCLEQ_NEXT(entry, hc_entry)) {
		if(entry->hc_name == name && entry->hc_len == len) {
			hashchain_unlock(entry);
			return (entry);
		}
	}
	hashchain_unlock(entry);
	return (NULL);
}

/* HTBC Hashchain/Blockchain HTree */

struct htree_root *
htbc_lookup_htree_root(chain)
	struct ht_hchain *chain;
{
	register struct htree_root *hroot;

	hroot = htbc_chain_lookup(chain->hc_name, chain->hc_len)->hc_hroot;
	return (hroot);
}

/* HTBC Hashchain/Blockchain Transactions */

struct ht_htransaction *
htbc_lookup_transaction(struct ht_hchain *chain)
{
	register struct ht_htransaction *trans;

	trans = htbc_chain_lookup(chain->hc_name, chain->hc_len)->hc_trans;
	return (trans);
}

/*
 * Knob to control the precision of file timestamps:
 *
 *   0 = seconds only; nanoseconds zeroed.
 *   1 = seconds and nanoseconds, accurate within 1/HZ.
 *   2 = seconds and nanoseconds, truncated to microseconds.
 * >=3 = seconds and nanoseconds, maximum precision.
 */
enum { TSP_SEC, TSP_HZ, TSP_USEC, TSP_NSEC };
static int timestamp_precision = TSP_USEC;

void
htbc_timestamp(struct timespec *tsp)
{
	struct timeval tv;
	switch (timestamp_precision) {
	case TSP_SEC:
		tsp->tv_sec = time_second;
		tsp->tv_nsec = 0;
		break;
	case TSP_HZ:
	case TSP_USEC:
		microtime(&tv);
		TIMEVAL_TO_TIMESPEC(&tv, tsp);
		break;
	case TSP_NSEC:
	default:
		break;
	}
}

void
htbc_add_timestamp_transaction(struct ht_htransaction *trans)
{
	struct timespec tsp;

	tsp = trans->ht_timespec;

	htbc_timestamp(&tsp);
	if(trans->ht_flag & IN_ACCESS) {
		trans->ht_atime = tsp.tv_sec;
		trans->ht_atimesec = tsp.tv_nsec;
	}
	if (trans->ht_flag & IN_UPDATE) {
		trans->ht_mtime = tsp.tv_sec;
		trans->ht_mtimesec = tsp.tv_nsec;
	}
	if (trans->ht_flag & IN_CHANGE) {
		trans->ht_ctime = tsp.tv_sec;
		trans->ht_ctimesec = tsp.tv_nsec;
	}
	trans->ht_flag &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE);
}

htbc_add_block_transaction(struct ht_htransaction *trans)
{

}

htbc_add_inode_transaction(struct ht_htransaction *trans)
{

}

/*
 * add transaction information to hashchain
 */
void
htbc_add_transaction(struct ht_hchain *chain)
{
	register struct ht_htransaction *trans;
	trans = chain->hc_trans;
	htbc_add_timestamp_transaction(trans);
	htbc_add_block_transaction(trans);
	htbc_add_inode_transaction(trans);

	trans->ht_reclen = (sizeof(*chain) + sizeof(*trans));
}

void
htbc_remove_transaction(struct ht_hchain *chain)
{
	register struct ht_htransaction *trans;

	trans = htbc_lookup_transaction(chain);
	if(trans != NULL) {
		trans = NULL;
	} else {
		printf("this entry does not hold any transaction information");
	}
}

/****************************************************************/
/* HTBC Inode */

void
htbc_inode_init(struct ht_htransaction *trans, u_int size)
{
	register union ht_ino *hti;
	hti = trans->ht_ino;
	if(hti == NULL) {
		hti = (union ht_ino *) malloc(size, M_HTBC, NULL);
	}
}

union ht_ino *
htbc_inode_get(chain, ino)
	struct ht_hchain *chain;
	ino_t ino;
{
	register struct ht_htransaction *trans;
	union ht_ino *hti;
	trans = htbc_lookup_transaction(chain);

	if(trans) {
		hti = trans->ht_ino;
		if(ino == hti->u_ino) {
			return (hti);
		}
	}
	return (NULL);
}

void
htbc_register_inode(chain, ino, mode)
	struct ht_hchain *chain;
	ino_t ino;
	mode_t mode;
{
	register struct ht_htransaction *trans;
	union ht_ino *hti;

	trans = htbc_lookup_transaction(chain);
	if(htbc_inode_get(chain, ino) == NULL) {
		hti->u_ino = ino;
		hti->u_imode = mode;
		trans->ht_inocnt++;
	}
}

void
htbc_unregister_inode(chain, ino, mode)
	struct ht_hchain *chain;
	ino_t ino;
	mode_t mode;
{
	register struct ht_htransaction *trans;
	union ht_ino *hti;

	trans = htbc_lookup_transaction(chain);
	hti = htbc_inode_get(ino, mode);
	if(hti) {
		KASSERT(hti == NULL);
		trans->ht_inocnt--;
	}
}

static __inline size_t
htbc_transaction_inodes_len(trans)
	struct ht_htransaction *trans;
{
	int blocklen = 1 << trans->ht_dev_bshift;
	int iph;

	/* Calculate number of inodes described in a inodelist header */
	iph = (blocklen - offsetof(struct ht_hinodelist, hc_inodes)) / sizeof(((struct ht_hinodelist *)0)->hc_inodes[0]);

	KASSERT(iph > 0);

	return MAX(1, howmany(trans->ht_inocnt, iph))*blocklen;
}

/* Calculate amount of space a transaction will take on disk */
static size_t
htbc_transaction_len(trans, bcount, bufcount, dealloccnt)
	struct ht_htransaction *trans;
	size_t bcount, bufcount;
	int dealloccnt;
{
	int blocklen = 1 << trans->ht_dev_bshift;
	size_t len;
	int bph;

	/* Calculate number of blocks described in a blocklist header */
	bph = (blocklen - offsetof(struct ht_hblocklist, hc_blocks)) / sizeof(((struct ht_hblocklist *)0)->hc_blocks[0]);

	KASSERT(bph > 0);

	len = bcount;
	len += howmany(bufcount, bph)*blocklen;
	len += howmany(dealloccnt, bph)*blocklen;
	len += htbc_transaction_inodes_len(trans);

	return (len);
}

int
htbc_directory_lookup(struct vnode *vdp, struct componentname *cnp, struct buf *bp)
{
	struct htbc_inode *dp = VTOHTI(vdp); 	/* inode for directory being searched */
	struct htree_direct *ep;				/* the current directory entry */
	int entryoffsetinblock;					/* offset of ep in bp's buffer */
	enum htbc_slotstatus slotstatus;
	off_t slotoffset;						/* offset of area with free space */
	int slotsize;							/* size of area at slotoffset */
	int slotfreespace;						/* amount of space free in slot */
	int slotneeded;							/* size of the entry we're seeking */
	int numdirpasses;						/* strategy for directory search */
	off_t prevoff;							/* prev entry dp->i_offset */
	off_t enduseful;						/* pointer past last used dir slot */
	u_long bmask;							/* block offset mask */
	int namlen, error;
	int flags;
	int nameiop = cnp->cn_nameiop;
	ino_t foundino;
	struct htree_lookup_results *results;

	flags = cnp->cn_flags;

	bp = NULL;
	slotoffset = -1;

	if ((error = VOP_ACCESS(vdp, VEXEC, cnp->cn_cred, vdp->v_proc)) != 0) {
		return (error);
	}

	slotstatus = FOUND;
	slotfreespace = slotsize = slotneeded = 0;
	if ((nameiop == CREATE || nameiop == RENAME) && (flags & ISLASTCN)) {
		slotstatus = NONE;
		slotneeded = HTREE_DIRSIZ(cnp->cn_namelen);
	}

	bmask = vdp->v_mount->mnt_stat.f_iosize - 1;
	if (nameiop != LOOKUP || results->hlr_diroff == 0 || results->hlr_diroff >= ext2fs_size(dp)) {
		entryoffsetinblock = 0;
		results->hlr_offset = 0;
		numdirpasses = 1;
	} else {
		results->hlr_offset = results->hlr_diroff;
		if ((entryoffsetinblock = results->hlr_offset & bmask) && (error = htbc_blkatoff(vdp, (off_t)results->hlr_offset, NULL, &bp))) {
			return (error);
		}
		numdirpasses = 2;
	}

	prevoff = results->hlr_offset;
	enduseful = 0;
	off_t i_offset;							/* cached i_offset value */
	struct htbc_searchslot ss;
	numdirpasses = 1;
	entryoffsetinblock = 0;

	int htree_lookup_ret = htree_lookup(dp, cnp->cn_nameptr, cnp->cn_namelen, &bp, &entryoffsetinblock, &i_offset, &prevoff, &enduseful, &ss);

	switch (htree_lookup_ret) {
	case 0:
		ep = (void *)((char *)bp->b_data + (i_offset & bmask));
		foundino = ep->h_ino;
		error = 0;
		break;
	case ENOENT:
		i_offset = roundup2(dp->hi_size, dp->hi_mfs->hi_bsize);
		error = ENOENT;
		break;
	}
	return (error);
}
