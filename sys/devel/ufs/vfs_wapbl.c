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
#include <ufs/wapbl.h>
#include <ufs/wapbl_replay.h>
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

	/* missing locks!! */

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

	SIMPLEQ_HEAD(, wapbl_entry) wl_entries; /* On disk transaction accounting */

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

static struct wapbl_entry *wapbl_entry;
static struct wapbl_dealloc *wapbl_dealloc;

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
	error = VOP_IOCTL(wl->wl_devvp, DIOCGCACHE, &wl->wl_dkcache,
	    FWRITE, FSCRED);
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
