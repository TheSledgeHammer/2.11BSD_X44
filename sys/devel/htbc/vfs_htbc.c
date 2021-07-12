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

/* Htree Blockchain*/

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

struct htbc_dealloc {
	TAILQ_ENTRY(htbc_dealloc) 					hd_entries;
	daddr_t 									hd_blkno;			/* address of block */
	int 										hd_len;				/* size of block */
};

struct htbc {
	struct vnode 								*ht_devvp;			/* this device */
	struct mount 								*ht_mount;			/* mountpoint ht is associated with */
	struct htbc_inode							*ht_inode;			/* inode */
	CIRCLEQ_HEAD(htbc_hashchain, htbc_hchain)	ht_hashchain;		/* hashchain contains all data pertaining to entries */

	daddr_t 									ht_pbn;				/* Physical block number of start */
	int 										ht_fs_dev_bshift;	/* logarithm of device block size of filesystem device */
	int 										ht_fs_dev_bsize;	/* logarithm of device block size of filesystem device */

	size_t 										ht_circ_size; 		/* Number of bytes in buffer of log */
	size_t 										ht_circ_off;		/* Number of bytes reserved at start */

	size_t 										ht_bufcount_max;
	size_t 										ht_bufbytes_max;


	void 										*ht_hc_scratch;		/* scratch space (XXX: por que?!?) */

	struct lock_object 							*ht_lock;			/* transaction lock */
	unsigned int 								ht_lock_count;		/* Count of transactions in progress */

	size_t 										ht_bufbytes;		/* Byte count of pages in ht_bufs */
	size_t 										ht_bufcount;		/* Count of buffers in ht_bufs */
	size_t 										ht_bcount;			/* Total bcount of ht_bufs */

	LIST_HEAD(, buf) 							ht_bufs; 			/* Buffers in current transaction */

	TAILQ_HEAD(, htbc_dealloc) 					ht_dealloclist;		/* list head */
	daddr_t 									*ht_deallocblks;	/* address of block */
	int 										*ht_dealloclens;	/* size of block */
	int 										ht_dealloccnt;		/* total count */
	int 										ht_dealloclim;		/* max count */
};

#define	HTBC_INODETRK_SIZE 83

static struct htbc_dealloc 						*htbc_dealloc;
struct htbc_hashchain							*htbc_blockchain;				/* add to htbc structure */

struct htbc_ops {
	void	(*ho_htbc_discard)(struct htbc *);
	void	(*ho_htbc_add_buf)(struct htbc *, struct buf *);
	void	(*ho_htbc_remove_buf)(struct htbc *, struct buf *);
	void	(*ho_htbc_resize_buf)(struct htbc *, struct buf *, long, long);
	int 	(*ho_htbc_begin)(struct htbc *, const char *, int);
	void 	(*ho_htbc_end)(struct htbc *);

	int		(*ho_htbc_read)(void *, size_t, struct vnode *, daddr_t);
	int		(*ho_htbc_write)(void *, size_t, struct vnode *, daddr_t);
};

struct htbc_ops htbc_ops = {
		.ho_htbc_add_buf = 		htbc_add_buf,
		.ho_htbc_remove_buf = 	htbc_remove_buf,
		.ho_htbc_resize_buf = 	htbc_resize_buf,
		.ho_htbc_begin = 		htbc_begin,
		.ho_htbc_end = 			htbc_end,
};

void
htbc_init()
{
	&htbc_dealloc = (struct htbc_dealloc *) malloc(sizeof(struct htbc_dealloc), M_HTBC, NULL);
}

void
htbc_fini()
{
	FREE(&htbc_dealloc, M_HTBC);
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
	daddr_t pbn;
	int error;
	int log_dev_bshift = DEV_BSHIFT;
	int fs_dev_bshift = DEV_BSHIFT;
	int run;

	if (log_dev_bshift > fs_dev_bshift) {
		return ENOSYS;
	}

	if (off < 0)
		return EINVAL;

	if (blksize < DEV_BSIZE)
		return EINVAL;
	if (blksize % DEV_BSIZE)
		return EINVAL;

	error = htbc_bmap(vp, off, &devvp, &pbn, &run);
	if (error != 0) {
		return (error);
	}

	ht = htbc_calloc(1, sizeof(*ht));
	simple_lock_init(ht->ht_lock, "htbc_lock");
	ht->ht_lock_count = 0;

	LIST_INIT(&ht->ht_bufs);
	CIRCLEQ_INIT(&ht->ht_hashchain);

	htip = ht->ht_inode;
	htip->hi_vnode = vp;
	ht->ht_devvp = htip->hi_devvp = devvp;
	ht->ht_mount = mp;
	ht->ht_pbn = pbn;
	ht->ht_fs_dev_bsize = log_dev_bshift;
	ht->ht_fs_dev_bshift = fs_dev_bshift;

	/* XXX fix actual number of pages reserved per filesystem. */
	ht->ht_bufbytes_max = MIN(ht->ht_circ_size, buf_memcalc() / 2);

	/* Round wl_bufbytes_max to the largest power of two constraint */
	ht->ht_bufbytes_max >>= PAGE_SHIFT;
	ht->ht_bufbytes_max <<= PAGE_SHIFT;
	ht->ht_bufbytes_max >>= ht->ht_fs_dev_bshift;
	ht->ht_bufbytes_max <<= ht->ht_fs_dev_bshift;

	/* XXX maybe use filesystem fragment size instead of 1024 */
	/* XXX fix actual number of buffers reserved per filesystem. */
	ht->ht_bufcount_max = (buf_nbuf() / 2) * 1024;

	/* XXX tie this into resource estimation */
	ht->ht_dealloclim = ht->ht_bufbytes_max / mp->mnt_stat.f_bsize / 2;

	ht->ht_deallocblks = htbc_alloc(sizeof(*ht->ht_deallocblks) * ht->ht_dealloclim);
	ht->ht_dealloclens = htbc_alloc(sizeof(*ht->ht_dealloclens) * ht->ht_dealloclim);

	htbc_inodetrk_init(ht, HTBC_INODETRK_SIZE);

	return (0);

//errout:
}

/*
 * Like wapbl_flush, only discards the transaction
 * completely
 */

void
htbc_discard(struct htbc *ht)
{
	struct buf *bp;
	int i;

	while ((bp = LIST_FIRST(&ht->ht_bufs)) != NULL) {

	}

	ht->ht_dealloccnt = 0;

	KASSERT(ht->ht_bufbytes == 0);
	KASSERT(ht->ht_bcount == 0);
	KASSERT(ht->ht_bufcount == 0);
	KASSERT(LIST_EMPTY(&ht->ht_bufs));
}

int
htbc_stop(struct htbc *ht, int force)
{
	struct vnode *vp;
	int error;
	error = htbc_flush(ht, 1);
	if (error) {
		if (force)
			htbc_discard(ht);
		else
			return error;
	}

	/* Unlinked inodes persist after a flush */
	if (ht->ht_inohashcnt) {
		if (force) {
			htbc_discard(ht);
		} else {
			return EBUSY;
		}
	}
	KASSERT(ht->ht_bufbytes == 0);
	KASSERT(ht->ht_bcount == 0);
	KASSERT(ht->ht_bufcount == 0);
	KASSERT(LIST_EMPTY(&ht->ht_bufs));
	KASSERT(ht->ht_dealloccnt == 0);
/*
	KASSERT(SIMPLEQ_EMPTY(&ht->ht_entries));
	KASSERT(ht->ht_inohashcnt == 0);

	vp = ht->ht_logvp;
*/
	htbc_discard(ht);
	htbc_free(ht->ht_hc_scratch);

	htbc_free(ht->ht_deallocblks);
	htbc_free(ht->ht_dealloclens);

	htbc_inodetrk_free(ht);
	htbc_free(ht);

	return (0);
}

int
htbc_doflush(ht, lockcount)
	struct htbc *ht;
	unsigned int lockcount;
{
	int doflush = ((ht->ht_bufbytes + (lockcount * MAXPHYS)) > ht->ht_bufbytes_max / 2) ||
			((ht->ht_bufcount + (lockcount * 10)) > ht->ht_bufcount_max / 2) ||
			(htbc_transaction_len(ht) > ht->ht_circ_size / 2) ||
			(ht->ht_dealloccnt >= (ht->ht_dealloclim / 2));

	return (doflush);
}

int
htbc_begin(struct htbc *ht, const char *file, int line)
{
	int doflush;
	unsigned int lockcount;

	KDASSERT(ht);

	htbc_lock(ht);
	lockcount = ht->ht_lock_count;
	doflush = htbc_doflush(ht, lockcount);
	htbc_unlock(ht);

	htbc_lock(ht);
	ht->ht_lock_count++;
	htbc_unlock(ht);

	return (0);
}

void
htbc_end(struct htbc *ht)
{
	htbc_lock(ht);
	KASSERT(ht->ht_lock_count > 0);
	ht->ht_lock_count--;
	htbc_unlock(ht);
}

void
htbc_add_buf(struct htbc *ht, struct buf *bp)
{
	KASSERT(bp->b_cflags & BC_BUSY);
	KASSERT(bp->b_vp);
	if (bp->b_flags & B_LOCKED) {
			LIST_REMOVE(bp, b_htbclist);
	} else {
		/* unlocked by dirty buffers shouldn't exist */
		KASSERT(!(bp->b_oflags & BO_DELWRI));
		ht->ht_bufbytes += bp->b_bufsize;
		ht->ht_bcount += bp->b_bcount;
		ht->ht_bufcount++;
	}
	LIST_INSERT_HEAD(&ht->ht_bufs, bp, b_htbclist);
	bp->b_flags |= B_LOCKED;
}

static void
htbc_remove_buf_locked(struct htbc *ht, struct buf *bp)
{
	KASSERT(bp->b_cflags & BC_BUSY);
#if 0
	/*
	 * XXX this might be an issue for swapfiles.
	 * see uvm_swap.c:1725
	 *
	 * XXXdeux: see above
	 */
	KASSERT((bp->b_flags & BC_NOCACHE) == 0);
#endif
	KASSERT(bp->b_flags & B_LOCKED);

	KASSERT(ht->ht_bufbytes >= bp->b_bufsize);
	ht->ht_bufbytes -= bp->b_bufsize;
	KASSERT(ht->ht_bcount >= bp->b_bcount);
	ht->ht_bcount -= bp->b_bcount;
	KASSERT(ht->ht_bufcount > 0);
	ht->ht_bufcount--;
	KASSERT((ht->ht_bufcount == 0) == (ht->ht_bufbytes == 0));
	KASSERT((ht->ht_bufcount == 0) == (ht->ht_bcount == 0));
	LIST_REMOVE(bp, b_htbclist);

	bp->b_flags &= ~B_LOCKED;
}

/* called from brelsel() in vfs_bio among other places */
void
htbc_remove_buf(struct htbc *ht, struct buf *bp)
{
	htbc_lock(ht);
	htbc_remove_buf_locked(ht, bp);
	htbc_unlock(ht);
}

void
htbc_resize_buf(struct htbc *ht, struct buf *bp, long oldsz, long oldcnt)
{
	KASSERT(bp->b_cflags & BC_BUSY);

	/*
	 * XXX: why does this depend on B_LOCKED?  otherwise the buf
	 * is not for a transaction?  if so, why is this called in the
	 * first place?
	 */
	if (bp->b_flags & B_LOCKED) {
		htbc_lock(ht);
		ht->ht_bufbytes += bp->b_bufsize - oldsz;
		ht->ht_bcount += bp->b_bcount - oldcnt;
		htbc_unlock(ht);
	}
}

/****************************************************************/
/* HTBC Unbuffered disk I/O */

static void
htbc_doio_accounting(devvp, flags)
	struct vnode *devvp;
	int flags;
{
	struct pstats *pstats = curproc->p_stats;

	if ((flags & (B_WRITE | B_READ)) == B_WRITE) {
		devvp->v_numoutput++;
		pstats->p_ru.ru_oublock++;
	} else {
		pstats->p_ru.ru_inblock++;
	}
}

static int
htbc_doio(data, len, devvp, pbn, flags)
	void *data;
	size_t len;
	struct vnode *devvp;
	daddr_t pbn;
	int flags;
{
	struct buf *bp;
	int error;

	KASSERT((flags & ~(B_WRITE | B_READ)) == 0);
	KASSERT(devvp->v_type == VBLK);

	htbc_doio_accounting(devvp, flags);

	bp->b_flags = flags;
	bp->b_cflags = BC_BUSY; /* silly & dubious */
	bp->b_dev = devvp->v_rdev;
	bp->b_data = data;
	bp->b_bufsize = bp->b_resid = bp->b_bcount = len;
	bp->b_blkno = pbn;

	VOP_STRATEGY(bp);

	error = biowait(bp);

	return (error);
}

int
htbc_write(data, len, devvp, pbn)
	void *data;
	size_t len;
	struct vnode *devvp;
	daddr_t pbn;
{
	return (htbc_doio(data, len, devvp, pbn, B_WRITE));
}

int
htbc_read(data, len, devvp, pbn)
	void *data;
	size_t len;
	struct vnode *devvp;
	daddr_t pbn;
{
	return (htbc_doio(data, len, devvp, pbn, B_READ));
}

/****************************************************************/
/* HTBC Hashchain */

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

void
get_hash_seed(struct htbc_inode *hi, uint32_t *seed)
{
	for (int i = 0; i < 4; i++) {
		seed[i] = hi->hi_hash_seed[i];
	}
}

struct htbc_hchain *
htbc_chain_lookup(name, len)
	const char 			*name;
	int 				len;
{
	struct htbc_hashchain 			*bucket;
	register struct htbc_hchain 	*entry;

	bucket = &htbc_blockchain[htbc_chain_hash(name, len)];
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

void
htbc_chain_insert(name, len)
	const char 			*name;
	int 				len;
{
	struct htbc_hashchain 			*bucket;
	register struct htbc_hchain 	*entry;

	bucket = &htbc_blockchain[htbc_chain_hash(name, len)];
	entry = (struct htbc_hchain *)malloc(sizeof(*entry), M_HTREE, M_WAITOK);
	entry->hc_trans = (struct htbc_htransaction *)malloc(sizeof(struct htbc_htransaction *), M_HTREE, M_WAITOK);
	entry->hc_hroot = (struct htree_root *)malloc(sizeof(struct htree_root *), M_HTREE, M_WAITOK);
	entry->hc_name = name;
	entry->hc_len = len;
	htbc_add_transaction(entry);

	hashchain_lock(entry);
	CIRCLEQ_INSERT_HEAD(bucket, entry, hc_entry);
	hashchain_unlock(entry);
	entry->hc_refcnt++;
}

void
htbc_chain_remove(name, len)
	const char 			*name;
	int 				len;
{
	struct htbc_hashchain 			*bucket;
	register struct htbc_hchain 	*entry;

	bucket = &htbc_blockchain[htbc_chain_hash(name, len)];
	for(entry = CIRCLEQ_FIRST(bucket); entry != NULL; entry = CIRCLEQ_NEXT(entry, hc_entry)) {
		if(entry->hc_name == name && entry->hc_len == len) {
			CIRCLEQ_REMOVE(bucket, entry, hc_entry);
			htbc_remove_transaction(entry);
			entry->hc_refcnt--;
			FREE(entry, M_HTREE);
			break;
		}
	}
}

/****************************************************************/
/* HTBC Transactions */

struct htree_root *
htbc_lookup_htree_root(struct htbc_hchain *chain)
{
	struct htbc_hchain *look;
	struct htree_root *hroot;

	look = htbc_chain_lookup(chain->hc_name, chain->hc_len);
	if(look) {
		hroot = look->hc_hroot;
		return (hroot);
	}

	return (NULL);
}

struct htbc_htransaction *
htbc_lookup_transaction(struct htbc_hchain *chain)
{
	struct htbc_hchain *look;
	struct htbc_htransaction *trans;

	look = htbc_chain_lookup(chain->hc_name, chain->hc_len);
	if(look) {
		trans = look->hc_trans;
		return (trans);
	}

	return (NULL);
}

void
htbc_add_transaction(struct htbc_hchain *chain)
{
	register struct htbc_htransaction *trans;
	trans = chain->hc_trans;
	htbc_add_timestamp_transaction(trans);
	htbc_add_block_transaction(trans);
	htbc_add_inode_transaction(trans);

	trans->ht_reclen = (sizeof(*chain) + sizeof(*trans));
}

void
htbc_remove_transaction(struct htbc_hchain *chain)
{
	register struct htbc_htransaction *trans;

	trans = htbc_lookup_transaction(chain);
	if(trans != NULL) {
		trans = NULL;
	} else {
		printf("this entry does not hold any transaction information");
	}
}

void
htbc_add_timestamp_transaction(struct htbc_htransaction *trans)
{
	struct timespec tsp;

	tsp = trans->ht_timespec;

	vfs_timestamp(&tsp);
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

void
htbc_add_block_transaction(struct htbc_htransaction *trans)
{

}

void
htbc_add_inode_transaction(struct htbc_htransaction *trans)
{

}

/****************************************************************/
/* HTBC Inode */

void
htbc_inodetrk_init(struct htbc_htransaction *trans, u_int size)
{
	register union ht_u_ino *hti;
	hti = trans->ht_ino;
	if(hti == NULL) {
		hti = (union ht_ino *) malloc(size, M_HTBC, NULL);
	}
}

union ht_ino *
htbc_inodetrk_get(chain, ino)
	struct htbc_hchain *chain;
	ino_t ino;
{
	register struct htbc_htransaction *trans;
	union ht_u_ino *hti;
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
	struct htbc_hchain *chain;
	ino_t ino;
	mode_t mode;
{
	register struct htbc_htransaction *trans;
	union ht_ino *hti;

	haschain_lock(chain);
	trans = htbc_lookup_transaction(chain);
	if(htbc_inode_get(chain, ino) == NULL) {
		hti->u_ino = ino;
		hti->u_imode = mode;
		trans->ht_inocnt++;
		haschain_unlock(chain);
	}
}

void
htbc_unregister_inode(chain, ino, mode)
	struct htbc_hchain *chain;
	ino_t ino;
	mode_t mode;
{
	register struct htbc_htransaction *trans;
	union ht_u_ino *hti;

	haschain_lock(chain);
	trans = htbc_lookup_transaction(chain);
	hti = htbc_inode_get(ino, mode);
	if(hti) {
		KASSERT(hti == NULL);
		trans->ht_inocnt--;
		haschain_unlock(chain);
	}
}

static __inline size_t
htbc_transaction_inodes_len(ht)
	struct htbc *ht;
{
	struct htbc_htransaction *trans;
	int blocklen;
	int iph;

	trans = htbc_lookup_transaction(ht->ht_hashchain);
	if (trans == NULL) {
		return (NOTRANSACTION);
	}

	blocklen = 1 << trans->ht_dev_bshift;

	/* Calculate number of inodes described in a inodelist header */
	iph = (blocklen - offsetof(struct htbc_hc_inodelist, hc_inodes)) / sizeof(((struct htbc_hc_inodelist *)0)->hc_inodes[0]);

	KASSERT(iph > 0);

	return MAX(1, howmany(trans->ht_inocnt, iph))*blocklen;
}

/* Calculate amount of space a transaction will take on disk */
static size_t
htbc_transaction_len(ht)
	struct htbc *ht;
{
	struct htbc_htransaction *trans;
	int blocklen;
	size_t len;
	int bph;

	trans = htbc_lookup_transaction(ht->ht_hashchain);
	if (trans == NULL) {
		return (NOTRANSACTION);
	}

	blocklen = 1 << trans->ht_dev_bshift;

	/* Calculate number of blocks described in a blocklist header */
	bph = (blocklen - offsetof(struct htbc_hc_blocklist, hc_blocks)) / sizeof(((struct htbc_hc_blocklist*) 0)->hc_blocks[0]);

	KASSERT(bph > 0);

	len = ht->ht_bcount;
	len += howmany(ht->ht_bufcount, bph)*blocklen;
	len += howmany(ht->ht_dealloccnt, bph)*blocklen;
	len += htbc_transaction_inodes_len(ht);
	return (len);
}

/****************************************************************/
/* HTBC Block */

/*
 * Return buffer with the contents of block "offset" from the beginning of
 * directory "ip".  If "res" is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
int
htbc_blkatoff(struct vnode *vp, off_t offset, char **res, struct buf **bpp)
{
	struct htbc_inode 	*ip;
	struct htbc_mfs 	*fs;
	struct buf *bp;
	daddr_t lbn;
	int error;

	ip = VTOHTI(vp);
	fs = ip->hi_mfs;
	lbn = htbc_lblkno(fs, offset);

	*bpp = NULL;
	if ((error = bread(vp, lbn, fs->hi_bsize, 0, &bp)) != 0) {
		return error;
	}
	if (res)
		*res = (char *)bp->b_data + htbc_blkoff(fs, offset);
	*bpp = bp;
	return 0;
}

/*
 * Bmap converts the logical block number of a file to its physical block
 * number on the disk. The conversion is done by using the logical block
 * number to index into the array of block pointers described by the htbc_inode.
 */
int
htbc_bmap(vp, bn, vpp, bnp, runp)
	struct vnode *vp;
	daddr_t  bn;
	struct vnode **vpp;
	daddr_t *bnp;
	int *runp;
{
	VTOHTI(vp)->hi_flag |= HTREE_EXTENTS;

	if(vpp != NULL) {
		vpp = VTOHTI(vp)->hi_devvp;
	}
	if(bnp == NULL) {
		return (0);
	}

	return (htbc_bmapext(vp, bn, bnp, runp, NULL));
}

/*
 * Convert the logical block number of a file to its physical block number
 * on the disk within htbc extents.
 */
int
htbc_bmapext(struct vnode *vp, int32_t bn, int64_t *bnp, int *runp, int *runb)
{
	struct htbc_mfs *fs;
	struct htbc_inode *ip;
	struct htbc_extent *ep;
	struct htbc_extent_path path = { .ep_bp = NULL };
	daddr_t lbn;
	int error = 0;

	ip = VTOHTI(vp);
	fs = ip->hi_mfs;
	lbn = bn;

	/* XXX: Should not initialize on error? */
	if (runp != NULL)
		*runp = 0;

	if (runb != NULL)
		*runb = 0;

	htbc_ext_find_extent(fs, ip, lbn, &path);
	if (path.ep_is_sparse) {
		*bnp = -1;
		if (runp != NULL)
			*runp = path.ep_sparse_ext.e_len - (lbn - path.ep_sparse_ext.e_blk)
					- 1;
		if (runb != NULL)
			*runb = lbn - path.ep_sparse_ext.e_blk;
	} else {
		if (path.ep_ext == NULL) {
			error = EIO;
			goto out;
		}
		ep = path.ep_ext;
		*bnp = htbc_fsbtodb(fs,
				lbn - ep->e_blk
						+ (ep->e_start_lo | (daddr_t) ep->e_start_hi << 32));

		if (*bnp == 0)
			*bnp = -1;

		if (runp != NULL)
			*runp = ep->e_len - (lbn - ep->e_blk) - 1;
		if (runb != NULL)
			*runb = lbn - ep->e_blk;
	}

out:
	if (path.ep_bp != NULL) {
		brelse(path.ep_bp, 0);
	}
	return (error);
}

/****************************************************************/
/* HTBC Extents & Caching */

static bool
htbc_ext_binsearch_index(struct htbc_inode *ip, struct htbc_extent_path *path, daddr_t lbn, daddr_t *first_lbn, daddr_t *last_lbn)
{
	struct htbc_extent_header *ehp = path->ep_header;
	struct htbc_extent_index *first, *last, *l, *r, *m;

	first = (struct htbc_extent_index *)(char *)(ehp + 1);
	last = first + ehp->eh_ecount - 1;
	l = first;
	r = last;
	while (l <= r) {
		m = l + (r - l) / 2;
		if (lbn < m->ei_blk)
			r = m - 1;
		else
			l = m + 1;
	}

	if (l == first) {
		path->ep_sparse_ext.e_blk = *first_lbn;
		path->ep_sparse_ext.e_len = first->ei_blk - *first_lbn;
		path->ep_sparse_ext.e_start_hi = 0;
		path->ep_sparse_ext.e_start_lo = 0;
		path->ep_is_sparse = TRUE;
		return TRUE;
	}
	path->ep_index = l - 1;
	*first_lbn = path->ep_index->ei_blk;
	if (path->ep_index < last)
		*last_lbn = l->ei_blk - 1;
	return FALSE;
}

static void
htbc_ext_binsearch(struct htbc_inode *ip, struct htbc_extent_path *path, daddr_t lbn, daddr_t first_lbn, daddr_t last_lbn)
{
	struct htbc_extent_header *ehp = path->ep_header;
	struct htbc_extent *first, *l, *r, *m;

	if (ehp->eh_ecount == 0)
		return;

	first = (struct htbc_extent *)(char *)(ehp + 1);
	l = first;
	r = first + ehp->eh_ecount - 1;
	while (l <= r) {
		m = l + (r - l) / 2;
		if (lbn < m->e_blk)
			r = m - 1;
		else
			l = m + 1;
	}

	if (l == first) {
		path->ep_sparse_ext.e_blk = first_lbn;
		path->ep_sparse_ext.e_len = first->e_blk - first_lbn;
		path->ep_sparse_ext.e_start_hi = 0;
		path->ep_sparse_ext.e_start_lo = 0;
		path->ep_is_sparse = TRUE;
		return;
	}
	path->ep_ext = l - 1;
	if (path->ep_ext->e_blk + path->ep_ext->e_len <= lbn) {
		path->ep_sparse_ext.e_blk = path->ep_ext->e_blk +
		    path->ep_ext->e_len;
		if (l <= (first + ehp->eh_ecount - 1))
			path->ep_sparse_ext.e_len = l->e_blk -
			    path->ep_sparse_ext.e_blk;
		else
			path->ep_sparse_ext.e_len = last_lbn -
			    path->ep_sparse_ext.e_blk + 1;
		path->ep_sparse_ext.e_start_hi = 0;
		path->ep_sparse_ext.e_start_lo = 0;
		path->ep_is_sparse = TRUE;
	}
}

/*
 * Find a block in htbc extent cache.
 */
int
htbc_ext_in_cache(struct htbc_inode *ip, daddr_t lbn, struct htbc_extent *ep)
{
	struct htbc_extent_cache *ecp;
	int ret = HTBC_EXT_CACHE_NO;

	ecp = &ip->hi_mfs->hi_ext.hi_ext_cache;

	/* cache is invalid */
	if (ecp->ec_type == HTBC_EXT_CACHE_NO)
		return ret;

	if (lbn >= ecp->ec_blk && lbn < ecp->ec_blk + ecp->ec_len) {
		ep->e_blk = ecp->ec_blk;
		ep->e_start_lo = ecp->ec_start & 0xffffffff;
		ep->e_start_hi = ecp->ec_start >> 32 & 0xffff;
		ep->e_len = ecp->ec_len;
		ret = ecp->ec_type;
	}
	return ret;
}

/*
 * Put an htbc_extent structure in htbc cache.
 */
void
htbc_ext_put_cache(struct htbc_inode *ip, struct htbc_extent *ep, int type)
{
	struct htbc_extent_cache *ecp;

	ecp = &ip->hi_mfs->hi_ext.hi_ext_cache;
	ecp->ec_type = type;
	ecp->ec_blk = ep->e_blk;
	ecp->ec_len = ep->e_len;
	ecp->ec_start = (daddr_t)ep->e_start_hi << 32 | ep->e_start_lo;
}

/*
 * Find an extent.
 */
struct htbc_extent_path *
htbc_ext_find_extent(struct htbc_mfs *fs, struct htbc_inode *ip, daddr_t lbn, struct htbc_extent_path *path)
{
	struct htbc_extent_header *ehp;
	uint16_t i;
	int error, size;
	daddr_t nblk;

	ehp = (struct htbc_extent_header *)ip->hi_blocks;

	if (ehp->eh_magic != HTBC_EXT_MAGIC)
		return NULL;

	path->ep_header = ehp;

	daddr_t first_lbn = 0;
	daddr_t last_lbn = htbc_lblkno(ip->hi_mfs, ip->hi_size);

	for (i = ehp->eh_depth; i != 0; --i) {
		path->ep_depth = i;
		path->ep_ext = NULL;
		if (htbc_ext_binsearch_index(ip, path, lbn, &first_lbn, &last_lbn)) {
			return path;
		}

		nblk = (daddr_t)path->ep_index->ei_leaf_hi << 32 |
		    path->ep_index->ei_leaf_lo;
		size = htbc_blksize(fs, ip, nblk);
		if (path->ep_bp != NULL) {
			brelse(path->ep_bp, 0);
			path->ep_bp = NULL;
		}
		error = bread(ip->hi_vnode, fsbtodb(fs, nblk), size, 0, &path->ep_bp);
		if (error) {
			brelse(path->ep_bp, 0);
			path->ep_bp = NULL;
			return NULL;
		}
		ehp = (struct htbc_extent_header *)path->ep_bp->b_data;
		path->ep_header = ehp;
	}

	path->ep_depth = i;
	path->ep_ext = NULL;
	path->ep_index = NULL;
	path->ep_is_sparse = FALSE;

	htbc_ext_binsearch(ip, path, lbn, first_lbn, last_lbn);
	return path;
}

/****************************************************************/
/* HTBC htree hash */

/*
 * FF, GG, and HH are transformations for rounds 1, 2, and 3.
 * Rotation is separated from addition to prevent recomputation.
 */

#define FF(a, b, c, d, x, s) { 								\
	(a) += F ((b), (c), (d)) + (x); 						\
	(a) = ROTATE_LEFT ((a), (s)); 							\
}

#define GG(a, b, c, d, x, s) { 								\
	(a) += G ((b), (c), (d)) + (x) + (uint32_t)0x5A827999; 	\
	(a) = ROTATE_LEFT ((a), (s)); 							\
}

#define HH(a, b, c, d, x, s) { 								\
	(a) += H ((b), (c), (d)) + (x) + (uint32_t)0x6ED9EBA1; 	\
	(a) = ROTATE_LEFT ((a), (s)); 							\
}

static void
htbc_prep_hashbuf(const char *src, int slen, uint32_t *dst, int dlen, int unsigned_char)
{
	uint32_t padding = slen | (slen << 8) | (slen << 16) | (slen << 24);
	uint32_t buf_val;
	const unsigned char *ubuf = (const unsigned char *)src;
	const signed char *sbuf = (const signed char *)src;
	int len, i;
	int buf_byte;

	if (slen > dlen)
		len = dlen;
	else
		len = slen;

	buf_val = padding;

	for (i = 0; i < len; i++) {
		if (unsigned_char)
			buf_byte = (u_int)ubuf[i];
		else
			buf_byte = (int)sbuf[i];

		if ((i % 4) == 0)
			buf_val = padding;

		buf_val <<= 8;
		buf_val += buf_byte;

		if ((i % 4) == 3) {
			*dst++ = buf_val;
			dlen -= sizeof(uint32_t);
			buf_val = padding;
		}
	}

	dlen -= sizeof(uint32_t);
	if (dlen >= 0)
		*dst++ = buf_val;

	dlen -= sizeof(uint32_t);
	while (dlen >= 0) {
		*dst++ = padding;
		dlen -= sizeof(uint32_t);
	}
}

static uint32_t
htbc_legacy_hash(const char *name, int len, int unsigned_char)
{
	uint32_t h0, h1 = 0x12A3FE2D, h2 = 0x37ABE8F9;
	uint32_t multi = 0x6D22F5;
	const unsigned char *uname = (const unsigned char *)name;
	const signed char *sname = (const signed char *)name;
	int val, i;

	for (i = 0; i < len; i++) {
		if (unsigned_char)
			val = (u_int)*uname++;
		else
			val = (int)*sname++;

		h0 = h2 + (h1 ^ (val * multi));
		if (h0 & 0x80000000)
			h0 -= 0x7FFFFFFF;
		h2 = h1;
		h1 = h0;
	}

	return h1 << 1;
}

/*
 * MD4 basic transformation.  It transforms state based on block.
 *
 * This is a half md4 algorithm since Linux uses this algorithm for dir
 * index.  This function is derived from the RSA Data Security, Inc. MD4
 * Message-Digest Algorithm and was modified as necessary.
 *
 * The return value of this function is uint32_t in Linux, but actually we don't
 * need to check this value, so in our version this function doesn't return any
 * value.
 */
static void
htbc_half_md4(uint32_t hash[4], uint32_t data[8])
{
	uint32_t a = hash[0], b = hash[1], c = hash[2], d = hash[3];

	/* Round 1 */
	FF(a, b, c, d, data[0],  3);
	FF(d, a, b, c, data[1],  7);
	FF(c, d, a, b, data[2], 11);
	FF(b, c, d, a, data[3], 19);
	FF(a, b, c, d, data[4],  3);
	FF(d, a, b, c, data[5],  7);
	FF(c, d, a, b, data[6], 11);
	FF(b, c, d, a, data[7], 19);

	/* Round 2 */
	GG(a, b, c, d, data[1],  3);
	GG(d, a, b, c, data[3],  5);
	GG(c, d, a, b, data[5],  9);
	GG(b, c, d, a, data[7], 13);
	GG(a, b, c, d, data[0],  3);
	GG(d, a, b, c, data[2],  5);
	GG(c, d, a, b, data[4],  9);
	GG(b, c, d, a, data[6], 13);

	/* Round 3 */
	HH(a, b, c, d, data[3],  3);
	HH(d, a, b, c, data[7],  9);
	HH(c, d, a, b, data[2], 11);
	HH(b, c, d, a, data[6], 15);
	HH(a, b, c, d, data[1],  3);
	HH(d, a, b, c, data[5],  9);
	HH(c, d, a, b, data[0], 11);
	HH(b, c, d, a, data[4], 15);

	hash[0] += a;
	hash[1] += b;
	hash[2] += c;
	hash[3] += d;
}

/*
 * Tiny Encryption Algorithm.
 */
static void
htbc_tea(uint32_t hash[4], uint32_t data[8])
{
	uint32_t tea_delta = 0x9E3779B9;
	uint32_t sum;
	uint32_t x = hash[0], y = hash[1];
	int n = 16;
	int i = 1;

	while (n-- > 0) {
		sum = i * tea_delta;
		x += ((y << 4) + data[0]) ^ (y + sum) ^ ((y >> 5) + data[1]);
		y += ((x << 4) + data[2]) ^ (x + sum) ^ ((x >> 5) + data[3]);
		i++;
	}

	hash[0] += x;
	hash[1] += y;
}

int
htbc_htree_hash(const char *name, int len, uint32_t *hash_seed, int hash_version, uint32_t *hash_major, uint32_t *hash_minor)
{
	uint32_t hash[4];
	uint32_t data[8];
	uint32_t major = 0, minor = 0;
	int unsigned_char = 0;

	if (!name || !hash_major)
		return -1;

	if (len < 1 || len > 255)
		goto error;

	hash[0] = 0x67452301;
	hash[1] = 0xEFCDAB89;
	hash[2] = 0x98BADCFE;
	hash[3] = 0x10325476;

	if (hash_seed)
		memcpy(hash, hash_seed, sizeof(hash));

	switch (hash_version) {
	case HTREE_TEA_UNSIGNED:
		unsigned char = 1;
		/* FALLTHROUGH */
	case HTREE_TEA:
		while (len > 0) {
			htbc_prep_hashbuf(name, len, data, 16, unsigned_char);
			htbc_tea(hash, data);
			len -= 16;
			name += 16;
		}
		major = hash[0];
		minor = hash[1];
		break;
	case HTREE_LEGACY_UNSIGNED:
		unsigned char = 1;
		/* FALLTHROUGH */
	case HTREE_LEGACY:
		major = htbc_legacy_hash(name, len, unsigned_char);
		break;
	case HTREE_HALF_MD4_UNSIGNED:
		unsigned char = 1;
		/* FALLTHROUGH */
	case HTREE_HALF_MD4:
		while (len > 0) {
			htbc_prep_hashbuf(name, len, data, 32, unsigned_char);
			htbc_half_md4(hash, data);
			len -= 32;
			name += 32;
		}
		major = hash[1];
		minor = hash[2];
		break;
	default:
		goto error;
	}

	major &= ~1;
	if (major == (HTREE_EOF << 1))
		major = (HTREE_EOF - 1) << 1;
	*hash_major = major;
	if (hash_minor)
		*hash_minor = minor;

	return 0;

error:
	*hash_major = 0;
	if (hash_minor)
		*hash_minor = 0;
	return -1;
}

/****************************************************************/
/* HTBC HTree */

static off_t
htree_get_block(struct htree_entry *ep)
{
	return (ep->h_blk & 0x00FFFFFF);
}

static void
htree_release(struct htree_lookup_info *info)
{
	for (u_int i = 0; i < info->h_levels_num; i++) {
		struct buf *bp = info->h_levels[i].h_bp;
		if (bp != NULL)
			brelse(bp, 0);
	}
}

static uint16_t
htree_get_limit(struct htree_entry *ep)
{
	return (((struct htree_count *)(ep))->h_entries_max);
}

static uint32_t
htree_root_limit(struct htbc_inode *ip, int len)
{
	uint32_t space = ip->hi_mfs->hi_bsize - HTREE_DIR_REC_LEN(1) - HTREE_DIR_REC_LEN(2) - len;
	return (space / sizeof(struct htree_entry));
}

static uint16_t
htree_get_count(struct htree_entry *ep)
{
	return ((struct htree_count *)(ep))->h_entries_num;
}

static uint32_t
htree_get_hash(struct htree_entry *ep)
{
	return ep->h_hash;
}

static void
htree_set_block(struct htree_entry *ep, uint32_t blk)
{
	ep->h_blk = blk;
}

static void
htree_set_count(struct htree_entry *ep, uint16_t cnt)
{
	((struct htree_count *)(ep))->h_entries_num = cnt;
}

static void
htree_set_hash(struct htree_entry *ep, uint32_t hash)
{
	ep->h_hash = hash;
}

static void
htree_set_limit(struct htree_entry *ep, uint16_t limit)
{
	((struct htree_count *)(ep))->h_entries_max = limit;
}

static uint32_t
htree_node_limit(struct htbc_inode *ip)
{
	struct htbc_mfs *fs;
	uint32_t space;

	fs = ip->hi_mfs;
	space = fs->hi_bsize - HTREE_DIR_REC_LEN(0);

	return (space / sizeof(struct htree_entry));
}

static int
htree_append_block(struct vnode *vp, char *data, struct componentname *cnp, uint32_t blksize)
{
	struct iovec aiov;
	struct uio auio;
	struct htbc_inode *dp = VTOHTI(vp);
	uint64_t cursize, newsize;
	int error;

	cursize = roundup(dp->hi_size, blksize);
	newsize = cursize + blksize;

	auio.uio_offset = cursize;
	auio.uio_resid = blksize;
	aiov.iov_len = blksize;
	aiov.iov_base = data;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_WRITE;
	auio.uio_vmspace = vp->v_proc->p_vmspace;
	error = VOP_WRITE(vp, &auio, IO_SYNC, cnp->cn_cred);
	if (!error)
		dp->hi_size = newsize;

	return (error);
}

static int
htree_writebuf(struct htree_lookup_info *info)
{
	int i, error;

	for (i = 0; i < info->h_levels_num; i++) {
		struct buf *bp = info->h_levels[i].h_bp;
		error = bwrite(bp);
		if (error)
			return (error);
	}

	return (0);
}

static void
htree_insert_entry_to_level(struct htree_lookup_level *level, uint32_t hash, uint32_t blk)
{
	struct htree_entry *target;
	int entries_num;

	target = level->h_entry + 1;
	entries_num = htree_get_count(level->h_entries);

	memmove(target + 1, target, (char *)(level->h_entries + entries_num) -
	    (char *)target);
	htree_set_block(target, blk);
	htree_set_hash(target, hash);
	htree_set_count(level->h_entries, entries_num + 1);
}

/*
 * Insert an index entry to the index node.
 */
static void
htree_insert_entry(struct htree_lookup_info *info, uint32_t hash, uint32_t blk)
{
	struct htree_lookup_level *level;

	level = &info->h_levels[info->h_levels_num - 1];
	htree_insert_entry_to_level(level, hash, blk);
}

/*
 * Compare two entry sort descriptors by name hash value.
 * This is used together with qsort.
 */
static int
htree_cmp_sort_entry(const void *e1, const void *e2)
{
	const struct htree_sort_entry *entry1, *entry2;

	entry1 = (const struct htree_sort_entry *)e1;
	entry2 = (const struct htree_sort_entry *)e2;

	if (entry1->h_hash < entry2->h_hash)
		return (-1);
	if (entry1->h_hash > entry2->h_hash)
		return (1);
	return (0);
}

/*
 * Append an entry to the end of the directory block.
 */
static void
htree_append_entry(char *block, uint32_t blksize, struct htree_direct *last_entry, struct htree_direct *new_entry)
{
	uint16_t entry_len;

	entry_len = HTREE_DIR_REC_LEN(last_entry->h_namlen);
	last_entry->h_reclen = entry_len;
	last_entry = (struct htree_direct *)((char *)last_entry + entry_len);
	new_entry->h_reclen = block + blksize - (char *)last_entry;
	memcpy(last_entry, new_entry, HTREE_DIR_REC_LEN(new_entry->h_namlen));
}

/*
 * Move half of entries from the old directory block to the new one.
 */
static int
htree_split_dirblock(char *block1, char *block2, uint32_t blksize, uint32_t *hash_seed, uint8_t hash_version, uint32_t *split_hash, struct htree_direct *entry)
{
	int entry_cnt = 0;
	int size = 0;
	int i, k;
	uint32_t offset;
	uint16_t entry_len = 0;
	uint32_t entry_hash;
	struct htree_direct *ep, *last;
	char *dest;
	struct htree_sort_entry *sort_info, dummy;

	ep = (struct htree_direct *)block1;
	dest = block2;
	sort_info = (struct htree_sort_entry *)
	    ((char *)block2 + blksize);

	/*
	 * Calculate name hash value for the entry which is to be added.
	 */
	htbc_htree_hash(entry->h_name, entry->h_namlen, hash_seed, hash_version, &entry_hash, NULL);

	/*
	 * Fill in directory entry sort descriptors.
	 */
	while ((char *)ep < block1 + blksize) {
		if (ep->h_ino && ep->h_namlen) {
			entry_cnt++;
			sort_info--;
			sort_info->h_size = ep->h_reclen;
			sort_info->h_offset = (char *)ep - block1;
			htbc_htree_hash(ep->h_name, ep->h_namlen, hash_seed, hash_version, &sort_info->h_hash, NULL);
		}
		ep = (struct htree_direct *)
		    ((char *)ep + ep->h_reclen);
	}

	/*
	 * Sort directory entry descriptors by name hash value.
	 */
	qsort(sort_info, entry_cnt, sizeof(struct htree_sort_entry), htree_cmp_sort_entry, &dummy);

	/*
	 * Count the number of entries to move to directory block 2.
	 */
	for (i = entry_cnt - 1; i >= 0; i--) {
		if (sort_info[i].h_size + size > blksize / 2)
			break;
		size += sort_info[i].h_size;
	}

	*split_hash = sort_info[i + 1].h_hash;

	/*
	 * Set collision bit.
	 */
	if (*split_hash == sort_info[i].h_hash)
		*split_hash += 1;

	/*
	 * Move half of directory entries from block 1 to block 2.
	 */
	for (k = i + 1; k < entry_cnt; k++) {
		ep = (struct htree_direct *)((char *)block1 +
		    sort_info[k].h_offset);
		entry_len = HTREE_DIR_REC_LEN(ep->h_namlen);
		memcpy(dest, ep, entry_len);
		((struct htree_direct *)dest)->h_reclen = entry_len;
		/* Mark directory entry as unused. */
		ep->h_ino = 0;
		dest += entry_len;
	}
	dest -= entry_len;

	/* Shrink directory entries in block 1. */
	last = (struct htree_direct *)block1;
	entry_len = 0;
	for (offset = 0; offset < blksize;) {
		ep = (struct htree_direct *)(block1 + offset);
		offset += ep->h_reclen;
		if (ep->h_ino) {
			last = (struct htree_direct *)
			   ((char *)last + entry_len);
			entry_len = HTREE_DIR_REC_LEN(ep->h_namlen);
			memcpy((void *)last, (void *)ep, entry_len);
			last->h_reclen = entry_len;
		}
	}

	if (entry_hash >= *split_hash) {
		/* Add entry to block 2. */
		htree_append_entry(block2, blksize,
		    (struct htree_direct *)dest, entry);

		/* Adjust length field of last entry of block 1. */
		last->h_reclen = block1 + blksize - (char *)last;
	} else {
		/* Add entry to block 1. */
		htree_append_entry(block1, blksize, last, entry);

		/* Adjust length field of last entry of block 2. */
		((struct htree_direct *)dest)->h_reclen =
		    block2 + blksize - dest;
	}

	return (0);
}

/*
 * Create an HTree index for a directory having entries which are no more
 * accommodable in a single dir-block.
 */
int
htree_create_index(struct vnode *vp, struct componentname *cnp, struct htree_direct *new_entry)
{
	struct buf *bp = NULL;
	struct htbc_inode *dp;
	struct htbc_mfs 	*mfs;
	struct htree_direct *ep, *dotdot;
	struct htree_root *root;
	struct htree_lookup_info info;
	uint32_t blksize, dirlen, split_hash;
	uint8_t hash_version;
	char *buf1 = NULL;
	char *buf2 = NULL;
	int error = 0;

	dp = VTOHTI(vp);
	mfs = dp->hi_mfs;
	blksize = mfs->hi_bsize;

	buf1 = malloc(blksize, M_TEMP, M_WAITOK | M_ZERO);
	buf2 = malloc(blksize, M_TEMP, M_WAITOK | M_ZERO);

	if ((error = htbc_blkatoff(vp, 0, NULL, &bp)) != 0)
		goto out;

	root = (struct htree_root *)bp->b_data;
	dotdot = (struct htree_direct *)((char *)&(root->h_dotdot));
	ep = (struct htree_direct *)((char *)dotdot + dotdot->h_reclen);
	dirlen = (char *)root + blksize - (char *)ep;
	memcpy(buf1, ep, dirlen);
	ep = (struct htree_direct *)buf1;
	while ((char *)ep < buf1 + dirlen)
		ep = (struct htree_direct *)((char *)ep + ep->h_reclen);
	ep->h_reclen = buf1 + blksize - (char *)ep;
	/* XXX It should be made dp->i_flag |= IN_E3INDEX; */
	dp->hi_sflags |= HTREE_INDEX;

	/*
	 * Initialize index root.
	 */
	dotdot->h_reclen = blksize - HTREE_DIR_REC_LEN(1);
	memset(&root->h_info, 0, sizeof(root->h_info));
	root->h_info.h_hash_version = dp->hi_def_hash_version;
	root->h_info.h_info_len = sizeof(root->h_info);
	htree_set_block(root->h_entries, 1);
	htree_set_count(root->h_entries, 1);
	htree_set_limit(root->h_entries, htree_root_limit(dp, sizeof(root->h_info)));

	memset(&info, 0, sizeof(info));
	info.h_levels_num = 1;
	info.h_levels[0].h_entries = root->h_entries;
	info.h_levels[0].h_entry = root->h_entries;

	hash_version = root->h_info.h_hash_version;
	if (hash_version <= HTREE_TEA)
		hash_version += mfs->hi_uhash;
	htree_split_dirblock(buf1, buf2, blksize, dp->hi_hash_seed,
	    hash_version, &split_hash, new_entry);
	htree_insert_entry(&info, split_hash, 2);

	/*
	 * Write directory block 0.
	 */
	if ((vp)->v_mount->mnt_flag & IO_SYNC)
		(void)bwrite(bp);
	else
		bdwrite(bp);

	dp->hi_flag |= IN_CHANGE | IN_UPDATE;

	/*
	 * Write directory block 1.
	 */
	error = htree_append_block(vp, buf1, cnp, blksize);
	if (error)
		goto out1;

	/*
	 * Write directory block 2.
	 */
	error = htree_append_block(vp, buf2, cnp, blksize);
	goto out1;
out:
	if (bp != NULL)
		brelse(bp, 0);
out1:
	free(buf1, M_TEMP);
	free(buf2, M_TEMP);
	return (error);
}

/*
 * Add an entry to the directory using htree index.
 */
int
htree_add_entry(struct vnode *dvp, struct htree_direct *entry, struct componentname *cnp, size_t newentrysize)
{
	struct htree_entry *entries, *leaf_node;
	struct htree_lookup_info info;
	struct buf *bp = NULL;
	struct htbc_mfs *mfs;
	struct htbc_inode *ip;
	uint16_t ent_num;
	uint32_t dirhash, split_hash;
	uint32_t blksize, blknum;
	uint64_t cursize, dirsize;
	uint8_t hash_version;
	char *newdirblock = NULL;
	char *newidxblock = NULL;
	struct htree_node *dst_node;
	struct htree_entry *dst_entries;
	struct htree_entry *root_entires;
	struct buf *dst_bp = NULL;
	int error, write_bp = 0, write_dst_bp = 0, write_info = 0;

	ip = VTOHTI(dvp);
	mfs = ip->hi_mfs;
	blksize = mfs->hi_bsize;

	if (ip->hi_count != 0)
		return htbc_add_entry(dvp, entry, &(ip), newentrysize);

	/* Target directory block is full, split it */
	memset(&info, 0, sizeof(info));
	error = htree_find_leaf(ip, entry->h_name, entry->h_namlen,
	    &dirhash, &hash_version, &info);
	if (error)
		return error;
	entries = info.h_levels[info.h_levels_num - 1].h_entries;
	ent_num = htree_get_count(entries);
	if (ent_num == htree_get_limit(entries)) {
		/* Split the index node. */
		root_entires = info.h_levels[0].h_entries;
		newidxblock = malloc(blksize, M_TEMP, M_WAITOK | M_ZERO);
		dst_node = (struct htree_node *)newidxblock;
		dst_entries = dst_node->h_entries;
		memset(&dst_node->h_fake_dirent, 0,
		    sizeof(dst_node->h_fake_dirent));
		dst_node->h_fake_dirent.h_reclen = blksize;

		cursize = roundup(ip->hi_size, blksize);
		dirsize = cursize + blksize;
		blknum = dirsize / blksize - 1;

		error = htree_append_block(dvp, newidxblock,
		    cnp, blksize);
		if (error)
			goto finish;
		error = htbc_blkatoff(dvp, cursize, NULL, &dst_bp);
		if (error)
			goto finish;

		dst_node = (struct htree_node *)dst_bp->b_data;
		dst_entries = dst_node->h_entries;

		if (info.h_levels_num == 2) {
			uint16_t src_ent_num, dst_ent_num;

			if (htree_get_count(root_entires) ==
			    htree_get_limit(root_entires)) {
				/* Directory index is full */
				error = EIO;
				goto finish;
			}

			src_ent_num = ent_num / 2;
			dst_ent_num = ent_num - src_ent_num;
			split_hash = htree_get_hash(entries + src_ent_num);

			/* Move half of index entries to the new index node */
			memcpy(dst_entries, entries + src_ent_num,
			    dst_ent_num * sizeof(struct htree_entry));
			htree_set_count(entries, src_ent_num);
			htree_set_count(dst_entries, dst_ent_num);
			htree_set_limit(dst_entries, htree_node_limit(ip));

			if (info.h_levels[1].h_entry >= entries + src_ent_num) {
				struct buf *tmp = info.h_levels[1].h_bp;
				info.h_levels[1].h_bp = dst_bp;
				dst_bp = tmp;

				info.h_levels[1].h_entry =
				    info.h_levels[1].h_entry -
				    (entries + src_ent_num) +
				    dst_entries;
				info.h_levels[1].h_entries = dst_entries;
			}
			htree_insert_entry_to_level(&info.h_levels[0], split_hash, blknum);

			/* Write new index node to disk */
			error = bwrite(dst_bp);
			ip->hi_flag |= IN_CHANGE | IN_UPDATE;
			if (error)
				goto finish;

			write_dst_bp = 1;
		} else {
			/* Create second level for htree index */

			struct htree_root *idx_root;

			memcpy(dst_entries, entries,
			    ent_num * sizeof(struct htree_entry));
			htree_set_limit(dst_entries, htree_node_limit(ip));

			idx_root = (struct htree_root *)
			    info.h_levels[0].h_bp->b_data;
			idx_root->h_info.h_ind_levels = 1;

			htree_set_count(entries, 1);
			htree_set_block(entries, blknum);

			info.h_levels_num = 2;
			info.h_levels[1].h_entries = dst_entries;
			info.h_levels[1].h_entry = info.h_levels[0].h_entry -
			    info.h_levels[0].h_entries + dst_entries;
			info.h_levels[1].h_bp = dst_bp;
			dst_bp = NULL;
		}
	}

	leaf_node = info.h_levels[info.h_levels_num - 1].h_entry;
	blknum = htree_get_block(leaf_node);
	error = htbc_blkatoff(dvp, blknum * blksize, NULL, &bp);
	if (error)
		goto finish;

	/* Split target directory block */
	newdirblock = malloc(blksize, M_TEMP, M_WAITOK | M_ZERO);
	htree_split_dirblock((char *)bp->b_data, newdirblock, blksize, ip->hi_hash_seed, hash_version, &split_hash, entry);
	cursize = roundup(ip->hi_size, blksize);
	dirsize = cursize + blksize;
	blknum = dirsize / blksize - 1;

	/* Add index entry for the new directory block */
	htree_insert_entry(&info, split_hash, blknum);

	/* Write the new directory block to the end of the directory */
	error = htree_append_block(dvp, newdirblock, cnp, blksize);

	if (error)
		goto finish;

	/* Write the target directory block */
	error = bwrite(bp);
	ip->hi_flag |= IN_CHANGE | IN_UPDATE;

	if (error)
		goto finish;
	write_bp = 1;

	/* Write the index block */
	error = htree_writebuf(&info);
	if (!error)
		write_info = 1;

finish:
	if (dst_bp != NULL && !write_dst_bp)
		brelse(dst_bp, 0);
	if (bp != NULL && !write_bp)
		brelse(bp, 0);
	if (newdirblock != NULL)
		free(newdirblock, M_TEMP);
	if (newidxblock != NULL)
		free(newidxblock, M_TEMP);
	if (!write_info)
		htree_release(&info);
	return error;
}

static int
htree_check_next(struct htbc_inode *ip, uint32_t hash, const char *name, struct htree_lookup_info *info)
{
	struct vnode *vp = HTITOV(ip);
	struct htree_lookup_level *level;
	struct buf *bp;
	uint32_t next_hash;
	int idx = info->h_levels_num - 1;
	int levels = 0;

	for (;;) {
		level = &info->h_levels[idx];
		level->h_entry++;
		if (level->h_entry < level->h_entries +
		    htree_get_count(level->h_entries))
			break;
		if (idx == 0)
			return 0;
		idx--;
		levels++;
	}

	next_hash = htree_get_hash(level->h_entry);
	if ((hash & 1) == 0) {
		if (hash != (next_hash & ~1))
			return 0;
	}

	while (levels > 0) {
		levels--;
		if (htbc_blkatoff(vp, htree_get_block(level->h_entry) *
		    ip->hi_mfs->hi_bsize, NULL, &bp) != 0)
			return 0;
		level = &info->h_levels[idx + 1];
		brelse(level->h_bp, 0);
		level->h_bp = bp;
		level->h_entry = level->h_entries =
		    ((struct htree_node *)bp->b_data)->h_entries;
	}

	return 1;
}

static int
htree_find_leaf(struct htbc_inode *ip, const char *name, int namelen, uint32_t *hash, uint8_t *hash_ver, struct htree_lookup_info *info)
{
	struct vnode *vp;
	struct htbc_mfs *mfs; /* F, G, and H are MD4 functions */
	struct buf *bp = NULL;
	struct htree_root *rootp;
	struct htree_entry *entp, *start, *end, *middle, *found;
	struct htree_lookup_level *level_info;
	uint32_t hash_major = 0, hash_minor = 0;
	uint32_t levels, cnt;
	uint8_t hash_version;

	if (name == NULL || info == NULL)
		return (-1);

	vp = HTITOV(ip);
	mfs = ip->hi_mfs;

	if (htbc_blkatoff(vp, 0, NULL, &bp) != 0)
		return (-1);

	info->h_levels_num = 1;
	info->h_levels[0].h_bp = bp;
	rootp = (struct htree_root *)bp->b_data;
	if (rootp->h_info.h_hash_version != HTREE_LEGACY &&
	    rootp->h_info.h_hash_version != HTREE_HALF_MD4 &&
	    rootp->h_info.h_hash_version != HTREE_TEA)
		goto error;

	hash_version = rootp->h_info.h_hash_version;
	if (hash_version <= HTREE_TEA)
		hash_version += mfs->hi_uhash;
	*hash_ver = hash_version;

	htbc_htree_hash(name, namelen, ip->hi_hash_seed,
	    hash_version, &hash_major, &hash_minor);
	*hash = hash_major;

	if ((levels = rootp->h_info.h_ind_levels) > 1)
		goto error;

	entp = (struct htree_entry *)(((char *)&rootp->h_info) +
	    rootp->h_info.h_info_len);

	if (htree_get_limit(entp) !=
	    htree_root_limit(ip, rootp->h_info.h_info_len))
		goto error;

	for (;;) {
		cnt = htree_get_count(entp);
		if (cnt == 0 || cnt > htree_get_limit(entp))
			goto error;

		start = entp + 1;
		end = entp + cnt - 1;
		while (start <= end) {
			middle = start + (end - start) / 2;
			if (htree_get_hash(middle) > hash_major)
				end = middle - 1;
			else
				start = middle + 1;
		}
		found = start - 1;

		level_info = &(info->h_levels[info->h_levels_num - 1]);
		level_info->h_bp = bp;
		level_info->h_entries = entp;
		level_info->h_entry = found;
		if (levels == 0)
			return (0);
		levels--;
		if (htbc_blkatoff(vp,
		    htree_get_block(found) * mfs->hi_bsize,
		    NULL, &bp) != 0)
			goto error;
		entp = ((struct htree_node *)bp->b_data)->h_entries;
		info->h_levels_num++;
		info->h_levels[info->h_levels_num - 1].h_bp = bp;
	}

error:
	htree_release(info);
	return (-1);
}

/*
 * Try to lookup a directory entry in HTree index
 */
int
htree_lookup(struct htbc_inode *ip, const char *name, int namelen, struct buf **bpp, int *entryoffp, int32_t *offp, int32_t *prevoffp, int32_t *endusefulp, struct htbc_searchslot *ss)
{
	struct vnode *vp;
	struct htree_lookup_info info;
	struct htree_entry *leaf_node;
	struct htbc_mfs *mfs;
	struct buf *bp;
	uint32_t blk;
	uint32_t dirhash;
	uint32_t bsize;
	uint8_t hash_version;
	int search_next;

	mfs = ip->hi_mfs;
	bsize = mfs->hi_bsize;
	vp = HTITOV(ip);

	/* TODO: print error msg because we don't lookup '.' and '..' */

	memset(&info, 0, sizeof(info));
	if (htree_find_leaf(ip, name, namelen, &dirhash, &hash_version, &info)) {
		return (-1);
	}

	do {
		leaf_node = info.h_levels[info.h_levels_num - 1].h_entry;
		blk = htree_get_block(leaf_node);
		if (htbc_blkatoff(vp, blk * bsize, NULL, &bp) != 0) {
			htree_release(&info);
			return (-1);
		}

		*offp = blk * bsize;
		*entryoffp = 0;
		*prevoffp = blk * bsize;
		*endusefulp = blk * bsize;

		if (ss->hi_slotstatus == NONE) {
			ss->hi_slotoffset = -1;
			ss->hi_slotfreespace = 0;
		}

		int found;
		if (htbc_search_dirblock(ip, bp->b_data, &found, name, namelen, entryoffp, offp, prevoffp,
		    endusefulp, ss) != 0) {
			brelse(bp, 0);
			htree_release(&info);
			return (-1);
		}

		if (found) {
			*bpp = bp;
			htree_release(&info);
			return (0);
		}

		brelse(bp, 0);
		search_next = htree_check_next(ip, dirhash, name, &info);
	} while (search_next);

	htree_release(&info);
	return (ENOENT);
}

int
htbc_add_entry(struct vnode* dvp, struct htree_direct *entry, const struct htree_lookup_results *hlr, size_t newentrysize)
{
	struct htree_direct *ep, *nep;
	struct htbc_inode *dp;
	struct buf *bp;
	u_int dsize;
	int error, loc, spacefree;
	char *dirbuf;

	dp = VTOHTI(dvp);

	/*
	 * Get the block containing the space for the new directory entry.
	 */
	if ((error = htbc_blkatoff(dvp, (off_t)hlr->hlr_offset, &dirbuf, &bp)) != 0)
			return error;

	/*
	 * Find space for the new entry. In the simple case, the entry at
	 * offset base will have the space. If it does not, then namei
	 * arranged that compacting the region ulr_offset to
	 * ulr_offset + ulr_count would yield the
	 * space.
	 */
	ep = (struct htree_direct*) dirbuf;
	dsize = HTREE_DIR_REC_LEN(ep->h_namlen);
	spacefree = fs2h16(ep->h_reclen) - dsize;
	for (loc = fs2h16(ep->h_reclen); loc < hlr->hlr_count;) {
		nep = (struct htree_direct*) (dirbuf + loc);
		if (ep->h_ino) {
			/* trim the existing slot */
			ep->h_reclen = h2fs16(dsize);
			ep = (struct htree_direct*) ((char*) ep + dsize);
		} else {
			/* overwrite; nothing there; header is ours */
			spacefree += dsize;
		}
		dsize = HTREE_DIR_REC_LEN(nep->h_namlen);
		spacefree += fs2h16(nep->h_reclen) - dsize;
		loc += fs2h16(nep->h_reclen);
		memcpy((void*) ep, (void*) nep, dsize);
	}
	/*
	 * Update the pointer fields in the previous entry (if any),
	 * copy in the new entry, and write out the block.
	 */
	if (ep->h_ino == 0) {
#ifdef DIAGNOSTIC
		if (spacefree + dsize < newentrysize)
			panic("htree_direnter: compact1");
#endif
		entry->h_reclen = h2fs16(spacefree + dsize);
	} else {
#ifdef DIAGNOSTIC
		if (spacefree < newentrysize) {
			printf("htree_direnter: compact2 %u %u",
			    (u_int)spacefree, (u_int)newentrysize);
			panic("htree_direnter: compact2");
		}
#endif
		entry->h_reclen = h2fs16(spacefree);
		ep->h_reclen = h2fs16(dsize);
		ep = (struct htree_direct*) ((char*) ep + dsize);
	}
	memcpy(ep, entry, (u_int) newentrysize);
	error = VOP_BWRITE(bp);
	dp->hi_flag |= IN_CHANGE | IN_UPDATE;
	return error;
}
