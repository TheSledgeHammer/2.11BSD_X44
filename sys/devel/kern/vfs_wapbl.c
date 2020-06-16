/*	$NetBSD: vfs_wapbl.c,v 1.103 2018/12/10 21:19:33 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
 * This implements file system independent write ahead filesystem logging.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <devel/sys/wapbl.h>
#include <devel/sys/wapbl_ops.h>
#include <devel/sys/wapbl_replay.h>
//#include <sys/bitops.h>

#ifdef _KERNEL

//#include <sys/atomic.h>
//#include <sys/evcnt.h>
//#include <sys/kauth.h>
//#include <sys/module.h>
//#include <sys/mutex.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/vnode.h>

#include <miscfs/specfs/specdev.h>

#define	wapbl_alloc(s, flags) 		malloc((s), M_WAPBL, flags)
#define	wapbl_free(a) 				free((a), M_WAPBL)
#define	wapbl_calloc(n, s, flags) 	malloc(((n)*(s)), M_WAPBL, flags)

static struct sysctllog *wapbl_sysctl;
static int wapbl_flush_disk_cache = 1;
static int wapbl_verbose_commit = 0;
static int wapbl_allow_dpofua = 0; 			/* switched off by default for now */
static int wapbl_journal_iobufs = 4;

static inline size_t wapbl_space_free(size_t, off_t, off_t);

#else /* !_KERNEL */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	KDASSERT(x) 		assert(x)
#define	KASSERT(x) 			assert(x)
#define	wapbl_alloc(s) 		malloc(s)
#define	wapbl_free(a, s) 	free(a)
#define	wapbl_calloc(n, s) 	calloc((n), (s))

#endif /* !_KERNEL */

#define wapbl_rwlock()		simplelock()
#define wapbl_rwunlock() 	simpleunlock()

/*
 * INTERNAL DATA STRUCTURES
 */

/*
 * This structure holds per-mount log information.
 *
 * Legend:	a = atomic access only
 *		r = read-only after init
 *		l = rwlock held
 *		m = mutex held
 *		lm = rwlock held writing or mutex held
 *		u = unlocked access ok
 *		b = bufcache_lock held
 */
LIST_HEAD(wapbl_ino_head, wapbl_ino);
struct wapbl {
	struct vnode *wl_logvp;	/* r:	log here */
	struct vnode *wl_devvp;	/* r:	log on this device */
	struct mount *wl_mount;	/* r:	mountpoint wl is associated with */
	daddr_t wl_logpbn;		/* r:	Physical block number of start of log */
	int wl_log_dev_bshift;	/* r:	logarithm of device block size of log device */
	int wl_fs_dev_bshift;	/* r:	logarithm of device block size of filesystem device */

	unsigned wl_lock_count;	/* m:	Count of transactions in progress */

	size_t wl_circ_size; 	/* r:	Number of bytes in buffer of log */
	size_t wl_circ_off;		/* r:	Number of bytes reserved at start */

	size_t wl_bufcount_max;	/* r:	Number of buffers reserved for log */
	size_t wl_bufbytes_max;	/* r:	Number of buf bytes reserved for log */

	off_t wl_head;			/* l:	Byte offset of log head */
	off_t wl_tail;			/* l:	Byte offset of log tail */

	/*
	 * WAPBL log layout, stored on wl_devvp at wl_logpbn:
	 *
	 *  ___________________ wl_circ_size __________________
	 * /                                                   \
	 * +---------+---------+-------+--------------+--------+
	 * [ commit0 | commit1 | CCWCW | EEEEEEEEEEEE | CCCWCW ]
	 * +---------+---------+-------+--------------+--------+
	 *       wl_circ_off --^       ^-- wl_head    ^-- wl_tail
	 *
	 * commit0 and commit1 are commit headers.  A commit header has
	 * a generation number, indicating which of the two headers is
	 * more recent, and an assignment of head and tail pointers.
	 * The rest is a circular queue of log records, starting at
	 * the byte offset wl_circ_off.
	 *
	 * E marks empty space for records.
	 * W marks records for block writes issued but waiting.
	 * C marks completed records.
	 *
	 * wapbl_flush writes new records to empty `E' spaces after
	 * wl_head from the current transaction in memory.
	 *
	 * wapbl_truncate advances wl_tail past any completed `C'
	 * records, freeing them up for use.
	 *
	 * head == tail == 0 means log is empty.
	 * head == tail != 0 means log is full.
	 *
	 * See assertions in wapbl_advance() for other boundary
	 * conditions.
	 *
	 * Only wapbl_flush moves the head, except when wapbl_truncate
	 * sets it to 0 to indicate that the log is empty.
	 *
	 * Only wapbl_truncate moves the tail, except when wapbl_flush
	 * sets it to wl_circ_off to indicate that the log is full.
	 */

	struct wapbl_wc_header *wl_wc_header;	/* l	*/
	void *wl_wc_scratch;	/* l:	scratch space (XXX: por que?!?) */


	struct simplelock wl_interlock; /* u:	short-term lock */

	/* missing locks!!: timedlock (kcondvar_t), rwlock & mutex */

	size_t wl_bufbytes;		/* m:	Byte count of pages in wl_bufs */
	size_t wl_bufcount;		/* m:	Count of buffers in wl_bufs */
	size_t wl_bcount;		/* m:	Total bcount of wl_bufs */

	TAILQ_HEAD(, buf) wl_bufs; /* m: Buffers in current transaction */

	kcondvar_t wl_reclaimable_cv;	/* m (obviously) */
	size_t wl_reclaimable_bytes; /* m:	Amount of space available for reclamation by truncate */

	int wl_error_count;	/* m:	# of wl_entries with errors */
	size_t wl_reserved_bytes; /* never truncate log smaller than this */

#ifdef WAPBL_DEBUG_BUFBYTES
	size_t wl_unsynced_bufbytes; /* Byte count of unsynced buffers */
#endif

#if _KERNEL
	int wl_brperjblock;	/* r Block records per journal block */
#endif

	TAILQ_HEAD(, wapbl_dealloc) wl_dealloclist;	/* lm:	list head */
	int wl_dealloccnt;							/* lm:	total count */
	int wl_dealloclim;							/* r:	max count */

	/* hashtable of inode numbers for allocated but unlinked inodes */
	/* synch ??? */
	struct wapbl_ino_head *wl_inohash;
	u_long wl_inohashmask;
	int wl_inohashcnt;

	TAILQ_HEAD(, wapbl_entry) wl_entries; /* On disk transaction accounting */

	/* buffers for wapbl_buffered_write() */
	TAILQ_HEAD(, buf) wl_iobufs;		/* l: Free or filling bufs */
	TAILQ_HEAD(, buf) wl_iobufs_busy;	/* l: In-transit bufs */

	int wl_dkcache;			/* r: 	disk cache flags */
#define WAPBL_USE_FUA(wl)		\
		(wapbl_allow_dpofua && ISSET((wl)->wl_dkcache, DKCACHE_FUA))
#define WAPBL_JFLAGS(wl)		\
		(WAPBL_USE_FUA(wl) ? (wl)->wl_jwrite_flags : 0)
#define WAPBL_JDATA_FLAGS(wl)	\
		(WAPBL_JFLAGS(wl) & B_MEDIA_DPO)	/* only DPO */
	int wl_jwrite_flags;	/* r: 	journal write flags */
};

#ifdef WAPBL_DEBUG_PRINT
int wapbl_debug_print = WAPBL_DEBUG_PRINT;
#endif

/****************************************************************/
#ifdef _KERNEL

#ifdef WAPBL_DEBUG
struct wapbl *wapbl_debug_wl;
#endif

static int wapbl_write_commit(struct wapbl *wl, off_t head, off_t tail);
static int wapbl_write_blocks(struct wapbl *wl, off_t *offp);
static int wapbl_write_revocations(struct wapbl *wl, off_t *offp);
static int wapbl_write_inodes(struct wapbl *wl, off_t *offp);
#endif /* _KERNEL */

static int wapbl_replay_process(struct wapbl_replay *wr, off_t, off_t);

static inline size_t wapbl_space_used(size_t avail, off_t head,
	off_t tail);

//#ifdef _KERNEL

static struct wapbl_entry *wapbl_entry;
static struct wapbl_dealloc *wapbl_dealloc;

#define	WAPBL_INODETRK_SIZE 83
static int wapbl_ino_pool_refcount;
//static struct pool wapbl_ino_pool;
struct wapbl_ino {
	LIST_ENTRY(wapbl_ino) wi_hash;
	ino_t wi_ino;
	mode_t wi_mode;
};

static void wapbl_inodetrk_init(struct wapbl *wl, u_int size);
static void wapbl_inodetrk_free(struct wapbl *wl);
static struct wapbl_ino *wapbl_inodetrk_get(struct wapbl *wl, ino_t ino);

static size_t wapbl_transaction_len(struct wapbl *wl);
static inline size_t wapbl_transaction_inodes_len(struct wapbl *wl);

static void wapbl_deallocation_free(struct wapbl *, struct wapbl_dealloc *,
	bool);

static void wapbl_evcnt_init(struct wapbl *);
static void wapbl_evcnt_free(struct wapbl *);

static void wapbl_dkcache_init(struct wapbl *);

#if 0
int wapbl_replay_verify(struct wapbl_replay *, struct vnode *);
#endif

static void
wapbl_init(void)
{
	&wapbl_entry = (struct wapbl_entry *) malloc(sizeof(struct wapbl_entry), M_WAPBL, NULL);
	&wapbl_dealloc = (struct wapbl_dealloc *) malloc(sizeof(struct wapbl_dealloc), M_WAPBL, NULL);
}

static int
wapbl_fini(void)
{
	FREE(&wapbl_dealloc, M_WAPBL);
	FREE(&wapbl_entry, M_WAPBL);
}

static void
wapbl_dkcache_init(wl)
	struct wapbl *wl;
{
	int error;

	/* Get disk cache flags */
	error = VOP_IOCTL(wl->wl_devvp, DIOCGCACHE, &wl->wl_dkcache, FWRITE, FSCRED);
	if (error) {
		/* behave as if there was a write cache */
		wl->wl_dkcache = DKCACHE_WRITE;
	}

	/* Use FUA instead of cache flush if available */
	if (ISSET(wl->wl_dkcache, DKCACHE_FUA))
		wl->wl_jwrite_flags |= B_MEDIA_FUA;

	/* Use DPO for journal writes if available */
	if (ISSET(wl->wl_dkcache, DKCACHE_DPO))
		wl->wl_jwrite_flags |= B_MEDIA_DPO;
}

static int
wapbl_start_flush_inodes(wl, wr)
	struct wapbl *wl;
	struct wapbl_replay *wr;
{
	int error, i;

	WAPBL_PRINTF(WAPBL_PRINT_REPLAY,
	    ("wapbl_start: reusing log with %d inodes\n", wr->wr_inodescnt));

	/*
	 * Its only valid to reuse the replay log if its
	 * the same as the new log we just opened.
	 */

	KDASSERT(!wapbl_replay_isopen(wr));
	KASSERT(wl->wl_devvp->v_type == VBLK);
	KASSERT(wr->wr_devvp->v_type == VBLK);
	KASSERT(wl->wl_devvp->v_rdev == wr->wr_devvp->v_rdev);
	KASSERT(wl->wl_logpbn == wr->wr_logpbn);
	KASSERT(wl->wl_circ_size == wr->wr_circ_size);
	KASSERT(wl->wl_circ_off == wr->wr_circ_off);
	KASSERT(wl->wl_log_dev_bshift == wr->wr_log_dev_bshift);
	KASSERT(wl->wl_fs_dev_bshift == wr->wr_fs_dev_bshift);

	wl->wl_wc_header->wc_generation = wr->wr_generation + 1;

	for (i = 0; i < wr->wr_inodescnt; i++)
		wapbl_register_inode(wl, wr->wr_inodes[i].wr_inumber,
		    wr->wr_inodes[i].wr_imode);

	/* Make sure new transaction won't overwrite old inodes list */
	KDASSERT(wapbl_transaction_len(wl) <=
	    wapbl_space_free(wl->wl_circ_size, wr->wr_inodeshead,
	    wr->wr_inodestail));

	wl->wl_head = wl->wl_tail = wr->wr_inodeshead;
	wl->wl_reclaimable_bytes = wl->wl_reserved_bytes =
	    wapbl_transaction_len(wl);

	error = wapbl_write_inodes(wl, &wl->wl_head);
	if (error)
		return error;

	KASSERT(wl->wl_head != wl->wl_tail);
	KASSERT(wl->wl_head != 0);

	return 0;
};

int
wapbl_start(wlp, mp, vp, off, count, blksize, wr, flushfn, flushabortfn)
	struct wapbl ** wlp;
	struct mount *mp;
	struct vnode *vp;
	daddr_t off;
	size_t count, blksize;
	struct wapbl_replay *wr;
	wapbl_flush_fn_t flushfn, flushabortfn;
{
	struct wapbl *wl;
	struct vnode *devvp;
	daddr_t logpbn;
	int error;
	int log_dev_bshift = ilog2(blksize);
	int fs_dev_bshift = log_dev_bshift;
	int run;

	WAPBL_PRINTF(WAPBL_PRINT_OPEN, ("wapbl_start: vp=%p off=%" PRId64
					" count=%zu blksize=%zu\n", vp, off, count, blksize));

	if (log_dev_bshift > fs_dev_bshift) {
		WAPBL_PRINTF(WAPBL_PRINT_OPEN,
				("wapbl: log device's block size cannot be larger "
						"than filesystem's\n"));
		/*
		 * Not currently implemented, although it could be if
		 * needed someday.
		 */
		return ENOSYS;
	}

	if (off < 0)
		return EINVAL;

	if (blksize < DEV_BSIZE)
		return EINVAL;
	if (blksize % DEV_BSIZE)
		return EINVAL;

	/* XXXTODO: verify that the full load is writable */

	/*
	 * XXX check for minimum log size
	 * minimum is governed by minimum amount of space
	 * to complete a transaction. (probably truncate)
	 */
	/* XXX for now pick something minimal */
	if ((count * blksize) < MAXPHYS) {
		return ENOSPC;
	}

	if ((error = VOP_BMAP(vp, off, &devvp, &logpbn, &run)) != 0) {
		return error;
	}

	wl = wapbl_calloc(1, sizeof(*wl));
	//rw_init(&wl->wl_rwlock);
	//mutex_init(&wl->wl_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&wl->wl_reclaimable_cv, "wapblrec");
	TAILQ_INIT(&wl->wl_bufs);
	SIMPLEQ_INIT(&wl->wl_entries);

	wl->wl_logvp = vp;
	wl->wl_devvp = devvp;
	wl->wl_mount = mp;
	wl->wl_logpbn = logpbn;
	wl->wl_log_dev_bshift = log_dev_bshift;
	wl->wl_fs_dev_bshift = fs_dev_bshift;

	wl->wl_flush = flushfn;
	wl->wl_flush_abort = flushabortfn;

	/* Reserve two log device blocks for the commit headers */
	wl->wl_circ_off = 2 << wl->wl_log_dev_bshift;
	wl->wl_circ_size = ((count * blksize) - wl->wl_circ_off);
	/* truncate the log usage to a multiple of log_dev_bshift */
	wl->wl_circ_size >>= wl->wl_log_dev_bshift;
	wl->wl_circ_size <<= wl->wl_log_dev_bshift;

	/*
	 * wl_bufbytes_max limits the size of the in memory transaction space.
	 * - Since buffers are allocated and accounted for in units of
	 *   PAGE_SIZE it is required to be a multiple of PAGE_SIZE
	 *   (i.e. 1<<PAGE_SHIFT)
	 * - Since the log device has to be written in units of
	 *   1<<wl_log_dev_bshift it is required to be a mulitple of
	 *   1<<wl_log_dev_bshift.
	 * - Since filesystem will provide data in units of 1<<wl_fs_dev_bshift,
	 *   it is convenient to be a multiple of 1<<wl_fs_dev_bshift.
	 * Therefore it must be multiple of the least common multiple of those
	 * three quantities.  Fortunately, all of those quantities are
	 * guaranteed to be a power of two, and the least common multiple of
	 * a set of numbers which are all powers of two is simply the maximum
	 * of those numbers.  Finally, the maximum logarithm of a power of two
	 * is the same as the log of the maximum power of two.  So we can do
	 * the following operations to size wl_bufbytes_max:
	 */

	/* XXX fix actual number of pages reserved per filesystem. */
	wl->wl_bufbytes_max = MIN(wl->wl_circ_size, buf_memcalc() / 2);

	/* Round wl_bufbytes_max to the largest power of two constraint */
	wl->wl_bufbytes_max >>= PAGE_SHIFT;
	wl->wl_bufbytes_max <<= PAGE_SHIFT;
	wl->wl_bufbytes_max >>= wl->wl_log_dev_bshift;
	wl->wl_bufbytes_max <<= wl->wl_log_dev_bshift;
	wl->wl_bufbytes_max >>= wl->wl_fs_dev_bshift;
	wl->wl_bufbytes_max <<= wl->wl_fs_dev_bshift;

	/* XXX maybe use filesystem fragment size instead of 1024 */
	/* XXX fix actual number of buffers reserved per filesystem. */
	wl->wl_bufcount_max = (buf_nbuf() / 2) * 1024;

	wl->wl_brperjblock = ((1<<wl->wl_log_dev_bshift) - offsetof(struct wapbl_wc_blocklist, wc_blocks)) / sizeof(((struct wapbl_wc_blocklist *)0)->wc_blocks[0]);
	KASSERT(wl->wl_brperjblock > 0);

	/* XXX tie this into resource estimation */
	wl->wl_dealloclim = wl->wl_bufbytes_max / mp->mnt_stat.f_bsize / 2;
	TAILQ_INIT(&wl->wl_dealloclist);

	wapbl_inodetrk_init(wl, WAPBL_INODETRK_SIZE);

	wapbl_evcnt_init(wl);

	wapbl_dkcache_init(wl);

	/* Initialize the commit header */
	{
		struct wapbl_wc_header *wc;
		size_t len = 1 << wl->wl_log_dev_bshift;
		wc = wapbl_calloc(1, len);
		wc->wc_type = WAPBL_WC_HEADER;
		wc->wc_len = len;
		wc->wc_circ_off = wl->wl_circ_off;
		wc->wc_circ_size = wl->wl_circ_size;
		/* XXX wc->wc_fsid */
		wc->wc_log_dev_bshift = wl->wl_log_dev_bshift;
		wc->wc_fs_dev_bshift = wl->wl_fs_dev_bshift;
		wl->wl_wc_header = wc;
		wl->wl_wc_scratch = wapbl_alloc(len);
	}

	TAILQ_INIT(&wl->wl_iobufs);
	TAILQ_INIT(&wl->wl_iobufs_busy);
	for (int i = 0; i < wapbl_journal_iobufs; i++) {
		struct buf *bp;

		if ((bp = geteblk(MAXPHYS)) == NULL)
			goto errout;

		//mutex_enter(&bufcache_lock);
		//mutex_enter(devvp->v_interlock);
		bgetvp(devvp, bp);
		//mutex_exit(devvp->v_interlock);
		//mutex_exit(&bufcache_lock);

		bp->b_dev = devvp->v_rdev;

		TAILQ_INSERT_TAIL(&wl->wl_iobufs, bp, b_wapbllist);
	}

	/*
	 * if there was an existing set of unlinked but
	 * allocated inodes, preserve it in the new
	 * log.
	 */
	if (wr && wr->wr_inodescnt) {
		error = wapbl_start_flush_inodes(wl, wr);
		if (error)
			goto errout;
	}

	error = wapbl_write_commit(wl, wl->wl_head, wl->wl_tail);
	if (error) {
		goto errout;
	}

	*wlp = wl;
#if defined(WAPBL_DEBUG)
			wapbl_debug_wl = wl;
		#endif

	return 0;
	errout: wapbl_discard(wl);
	wapbl_free(wl->wl_wc_scratch, wl->wl_wc_header->wc_len);
	wapbl_free(wl->wl_wc_header, wl->wl_wc_header->wc_len);
	while (!TAILQ_EMPTY(&wl->wl_iobufs)) {
		struct buf *bp;

		bp = TAILQ_FIRST(&wl->wl_iobufs);
		TAILQ_REMOVE(&wl->wl_iobufs, bp, b_wapbllist);
		brelse(bp, BC_INVAL);
	}
	wapbl_inodetrk_free(wl);
	wapbl_free(wl, sizeof(*wl));

	return error;
}

/*
 * Like wapbl_flush, only discards the transaction
 * completely
 */

void
wapbl_discard(struct wapbl *wl)
{
	struct wapbl_entry *we;
	struct wapbl_dealloc *wd;
	struct buf *bp;
	int i;

	/*
	 * XXX we may consider using upgrade here
	 * if we want to call flush from inside a transaction
	 */
	//rw_enter(&wl->wl_rwlock, RW_WRITER);
	wl->wl_flush(wl->wl_mount, TAILQ_FIRST(&wl->wl_dealloclist));

#ifdef WAPBL_DEBUG_PRINT
	{
		pid_t pid = -1;
		lwpid_t lid = -1;
		if (curproc)
			pid = curproc->p_pid;
		if (curlwp)
			lid = curlwp->l_lid;
#ifdef WAPBL_DEBUG_BUFBYTES
		WAPBL_PRINTF(WAPBL_PRINT_DISCARD,
		    ("wapbl_discard: thread %d.%d discarding "
		    "transaction\n"
		    "\tbufcount=%zu bufbytes=%zu bcount=%zu "
		    "deallocs=%d inodes=%d\n"
		    "\terrcnt = %u, reclaimable=%zu reserved=%zu "
		    "unsynced=%zu\n",
		    pid, lid, wl->wl_bufcount, wl->wl_bufbytes,
		    wl->wl_bcount, wl->wl_dealloccnt,
		    wl->wl_inohashcnt, wl->wl_error_count,
		    wl->wl_reclaimable_bytes, wl->wl_reserved_bytes,
		    wl->wl_unsynced_bufbytes));
		SIMPLEQ_FOREACH(we, &wl->wl_entries, we_entries) {
			WAPBL_PRINTF(WAPBL_PRINT_DISCARD,
			    ("\tentry: bufcount = %zu, reclaimable = %zu, "
			     "error = %d, unsynced = %zu\n",
			     we->we_bufcount, we->we_reclaimable_bytes,
			     we->we_error, we->we_unsynced_bufbytes));
		}
#else /* !WAPBL_DEBUG_BUFBYTES */
		WAPBL_PRINTF(WAPBL_PRINT_DISCARD,
		    ("wapbl_discard: thread %d.%d discarding transaction\n"
		    "\tbufcount=%zu bufbytes=%zu bcount=%zu "
		    "deallocs=%d inodes=%d\n"
		    "\terrcnt = %u, reclaimable=%zu reserved=%zu\n",
		    pid, lid, wl->wl_bufcount, wl->wl_bufbytes,
		    wl->wl_bcount, wl->wl_dealloccnt,
		    wl->wl_inohashcnt, wl->wl_error_count,
		    wl->wl_reclaimable_bytes, wl->wl_reserved_bytes));
		SIMPLEQ_FOREACH(we, &wl->wl_entries, we_entries) {
			WAPBL_PRINTF(WAPBL_PRINT_DISCARD,
			    ("\tentry: bufcount = %zu, reclaimable = %zu, "
			     "error = %d\n",
			     we->we_bufcount, we->we_reclaimable_bytes,
			     we->we_error));
		}
#endif /* !WAPBL_DEBUG_BUFBYTES */
	}
#endif /* WAPBL_DEBUG_PRINT */

	for (i = 0; i <= wl->wl_inohashmask; i++) {
		struct wapbl_ino_head *wih;
		struct wapbl_ino *wi;

		wih = &wl->wl_inohash[i];
		while ((wi = LIST_FIRST(wih)) != NULL) {
			LIST_REMOVE(wi, wi_hash);
			pool_put(&wapbl_ino_pool, wi);
			KASSERT(wl->wl_inohashcnt > 0);
			wl->wl_inohashcnt--;
		}
	}

	/*
	 * clean buffer list
	 */
	mutex_enter(&bufcache_lock);
	mutex_enter(&wl->wl_mtx);
	while ((bp = TAILQ_FIRST(&wl->wl_bufs)) != NULL) {
		if (bbusy(bp, 0, 0, &wl->wl_mtx) == 0) {
			KASSERT(bp->b_flags & B_LOCKED);
			KASSERT(bp->b_oflags & BO_DELWRI);
			/*
			 * Buffer is already on BQ_LOCKED queue.
			 * The buffer will be unlocked and
			 * removed from the transaction in brelsel()
			 */
			//mutex_exit(&wl->wl_mtx);
			bremfree(bp);
			brelsel(bp, BC_INVAL);
			//mutex_enter(&wl->wl_mtx);
		}
	}

	/*
	 * Remove references to this wl from wl_entries, free any which
	 * no longer have buffers, others will be freed in wapbl_biodone()
	 * when they no longer have any buffers.
	 */
	while ((we = SIMPLEQ_FIRST(&wl->wl_entries)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&wl->wl_entries, we_entries);
		/* XXX should we be accumulating wl_error_count
		 * and increasing reclaimable bytes ? */
		we->we_wapbl = NULL;
		if (we->we_bufcount == 0) {
#ifdef WAPBL_DEBUG_BUFBYTES
			KASSERT(we->we_unsynced_bufbytes == 0);
#endif
			pool_put(&wapbl_entry, we);
		}
	}

	//mutex_exit(&wl->wl_mtx);
	//mutex_exit(&bufcache_lock);

	/* Discard list of deallocs */
	while ((wd = TAILQ_FIRST(&wl->wl_dealloclist)) != NULL)
		wapbl_deallocation_free(wl, wd, true);

	/* XXX should we clear wl_reserved_bytes? */

	KASSERT(wl->wl_bufbytes == 0);
	KASSERT(wl->wl_bcount == 0);
	KASSERT(wl->wl_bufcount == 0);
	KASSERT(TAILQ_EMPTY(&wl->wl_bufs));
	KASSERT(SIMPLEQ_EMPTY(&wl->wl_entries));
	KASSERT(wl->wl_inohashcnt == 0);
	KASSERT(TAILQ_EMPTY(&wl->wl_dealloclist));
	KASSERT(wl->wl_dealloccnt == 0);

	//rw_exit(&wl->wl_rwlock);
}

int
wapbl_stop(wl, force)
	struct wapbl *wl;
	int force;
{
	int error;

	WAPBL_PRINTF(WAPBL_PRINT_OPEN, ("wapbl_stop called\n"));
	error = wapbl_flush(wl, 1);
	if (error) {
		if (force)
			wapbl_discard(wl);
		else
			return error;
	}

	/* Unlinked inodes persist after a flush */
	if (wl->wl_inohashcnt) {
		if (force) {
			wapbl_discard(wl);
		} else {
			return EBUSY;
		}
	}

	KASSERT(wl->wl_bufbytes == 0);
	KASSERT(wl->wl_bcount == 0);
	KASSERT(wl->wl_bufcount == 0);
	KASSERT(TAILQ_EMPTY(&wl->wl_bufs));
	KASSERT(wl->wl_dealloccnt == 0);
	KASSERT(SIMPLEQ_EMPTY(&wl->wl_entries));
	KASSERT(wl->wl_inohashcnt == 0);
	KASSERT(TAILQ_EMPTY(&wl->wl_dealloclist));
	KASSERT(wl->wl_dealloccnt == 0);
	KASSERT(TAILQ_EMPTY(&wl->wl_iobufs_busy));

	wapbl_free(wl->wl_wc_scratch, wl->wl_wc_header->wc_len);
	wapbl_free(wl->wl_wc_header, wl->wl_wc_header->wc_len);
	while (!TAILQ_EMPTY(&wl->wl_iobufs)) {
		struct buf *bp;

		bp = TAILQ_FIRST(&wl->wl_iobufs);
		TAILQ_REMOVE(&wl->wl_iobufs, bp, b_wapbllist);
		brelse(bp, BC_INVAL);
	}
	wapbl_inodetrk_free(wl);

	wapbl_evcnt_free(wl);

	cv_destroy(&wl->wl_reclaimable_cv);
	//mutex_destroy(&wl->wl_mtx);
	//rw_destroy(&wl->wl_rwlock);
	wapbl_free(wl, sizeof(*wl));

	return 0;
}

/****************************************************************/
/*
 * Unbuffered disk I/O
 */

static void
wapbl_doio_accounting(devvp, flags)
	struct vnode *devvp;
	int flags;
{
	struct pstats *pstats = curproc->p_stats;

	if ((flags & (B_WRITE | B_READ)) == B_WRITE) {
	//	mutex_enter(devvp->v_interlock);
		devvp->v_numoutput++;
	//	mutex_exit(devvp->v_interlock);
		pstats->p_ru.ru_oublock++;
	} else {
		pstats->p_ru.ru_inblock++;
	}

}

static int
wapbl_doio(data, len, devvp, pbn, flags)
	void *data;
	size_t len;
	struct vnode *devvp;
	daddr_t pbn;
	int flags;
{
	struct buf *bp;
	int error;

	KASSERT(devvp->v_type == VBLK);

	wapbl_doio_accounting(devvp, flags);

	bp = getiobuf(devvp, true);
	bp->b_flags = flags;
	bp->b_cflags |= BC_BUSY;	/* mandatory, asserted by biowait() */
	bp->b_dev = devvp->v_rdev;
	bp->b_data = data;
	bp->b_bufsize = bp->b_resid = bp->b_bcount = len;
	bp->b_blkno = pbn;
	BIO_SETPRIO(bp, BPRIO_TIMECRITICAL);

	WAPBL_PRINTF(WAPBL_PRINT_IO,
	    ("wapbl_doio: %s %d bytes at block %"PRId64" on dev 0x%"PRIx64"\n",
	    BUF_ISWRITE(bp) ? "write" : "read", bp->b_bcount,
	    bp->b_blkno, bp->b_dev));

	VOP_STRATEGY(bp);

	error = biowait(bp);
	putiobuf(bp);

	if (error) {
		WAPBL_PRINTF(WAPBL_PRINT_ERROR,
		    ("wapbl_doio: %s %zu bytes at block %" PRId64
		    " on dev 0x%"PRIx64" failed with error %d\n",
		    (((flags & (B_WRITE | B_READ)) == B_WRITE) ?
		     "write" : "read"),
		    len, pbn, devvp->v_rdev, error));
	}

	return error;
}

/*
 * wapbl_write(data, len, devvp, pbn)
 *
 *	Synchronously write len bytes from data to physical block pbn
 *	on devvp.
 */
int
wapbl_write(data, len, devvp, pbn)
	void *data;
	size_t len;
	struct vnode *devvp;
	daddr_t pbn;
{
	return wapbl_doio(data, len, devvp, pbn, B_WRITE);
}

/*
 * wapbl_read(data, len, devvp, pbn)
 *
 *	Synchronously read len bytes into data from physical block pbn
 *	on devvp.
 */
int
wapbl_read(data, len, devvp, pbn)
	void *data;
	size_t len;
	struct vnode *devvp;
	daddr_t pbn;
{
	return wapbl_doio(data, len, devvp, pbn, B_READ);
}
