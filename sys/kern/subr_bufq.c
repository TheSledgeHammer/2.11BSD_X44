/*	$NetBSD: subr_disk.c,v 1.60 2004/03/09 12:23:07 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	@(#)ufs_disksubr.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/disk.h>
#include <sys/stdbool.h>
#include <sys/syslog.h>
#include <sys/tree.h>

struct bufq_fcfs {
	TAILQ_HEAD(, buf) 	bq_head;			/* actual list of buffers */
};

struct bufq_disksort {
	TAILQ_HEAD(, buf) 	bq_head;			/* actual list of buffers */
};

#define PRIO_READ_BURST	48
#define PRIO_WRITE_REQ	16

struct bufq_prio {
	TAILQ_HEAD(, buf) 	bq_read; 			/* actual list of buffers */
	TAILQ_HEAD(, buf)	bq_write;
	struct buf 			*bq_write_next;		/* next request in bq_write */
	struct buf 			*bq_next;			/* current request */
	int 				bq_read_burst;		/* # of consecutive reads */
};

static __inline int buf_inorder(const struct buf *, const struct buf *, int);

/*
 * Check if two buf's are in ascending order.
 */
static __inline int
buf_inorder(bp, bq, sortby)
	const struct buf *bp;
	const struct buf *bq;
	int sortby;
{
	if (bp == NULL || bq == NULL)
		return (bq == NULL);

	if (sortby == BUFQ_SORT_CYLINDER) {
		if (bp->b_cylin != bq->b_cylin)
			return bp->b_cylin < bq->b_cylin;
		else
			return bp->b_rawblkno < bq->b_rawblkno;
	} else
		return bp->b_rawblkno < bq->b_rawblkno;
}

/*
 * First-come first-served sort for disks.
 *
 * Requests are appended to the queue without any reordering.
 */
static void
bufq_fcfs_put(bufq, bp)
	struct bufq_state *bufq;
	struct buf *bp;
{
	struct bufq_fcfs *fcfs = bufq->bq_private;

	TAILQ_INSERT_TAIL(&fcfs->bq_head, bp, b_actq);
}

static struct buf *
bufq_fcfs_get(bufq, remove)
	struct bufq_state *bufq;
	int remove;
{
	struct bufq_fcfs *fcfs = bufq->bq_private;
	struct buf *bp;

	bp = TAILQ_FIRST(&fcfs->bq_head);

	if (bp != NULL && remove)
		TAILQ_REMOVE(&fcfs->bq_head, bp, b_actq);

	return (bp);
}

/*
 * Seek sort for disks.
 *
 * There are actually two queues, sorted in ascendening order.  The first
 * queue holds those requests which are positioned after the current block;
 * the second holds requests which came in after their position was passed.
 * Thus we implement a one-way scan, retracting after reaching the end of
 * the drive to the first request on the second queue, at which time it
 * becomes the first queue.
 *
 * A one-way scan is natural because of the way UNIX read-ahead blocks are
 * allocated.
 */
static void
bufq_disksort_put(bufq, bp)
	struct bufq_state *bufq;
	struct buf *bp;
{
	struct bufq_disksort *disksort = bufq->bq_private;
	struct buf *bq, *nbq;
	int sortby;

	sortby = bufq->bq_flags & BUFQ_SORT_MASK;

	bq = TAILQ_FIRST(&disksort->bq_head);

	/*
	 * If the queue is empty it's easy; we just go on the end.
	 */
	if (bq == NULL) {
		TAILQ_INSERT_TAIL(&disksort->bq_head, bp, b_actq);
		return;
	}

	/*
	 * If we lie before the currently active request, then we
	 * must locate the second request list and add ourselves to it.
	 */
	if (buf_inorder(bp, bq, sortby)) {
		while ((nbq = TAILQ_NEXT(bq, b_actq)) != NULL) {
			/*
			 * Check for an ``inversion'' in the normally ascending
			 * block numbers, indicating the start of the second
			 * request list.
			 */
			if (buf_inorder(nbq, bq, sortby)) {
				/*
				 * Search the second request list for the first
				 * request at a larger block number.  We go
				 * after that; if there is no such request, we
				 * go at the end.
				 */
				do {
					if (buf_inorder(bp, nbq, sortby))
						goto insert;
					bq = nbq;
				} while ((nbq =
				    TAILQ_NEXT(bq, b_actq)) != NULL);
				goto insert;		/* after last */
			}
			bq = nbq;
		}
		/*
		 * No inversions... we will go after the last, and
		 * be the first request in the second request list.
		 */
		goto insert;
	}
	/*
	 * Request is at/after the current request...
	 * sort in the first request list.
	 */
	while ((nbq = TAILQ_NEXT(bq, b_actq)) != NULL) {
		/*
		 * We want to go after the current request if there is an
		 * inversion after it (i.e. it is the end of the first
		 * request list), or if the next request is a larger cylinder
		 * than our request.
		 */
		if (buf_inorder(nbq, bq, sortby) || buf_inorder(bp, nbq, sortby))
			goto insert;
		bq = nbq;
	}
	/*
	 * Neither a second list nor a larger request... we go at the end of
	 * the first list, which is the same as the end of the whole schebang.
	 */
insert:
	TAILQ_INSERT_AFTER(&disksort->bq_head, bq, bp, b_actq);
}

static struct buf *
bufq_disksort_get(bufq, remove)
	struct bufq_state *bufq;
	int remove;
{
	struct bufq_disksort *disksort = bufq->bq_private;
	struct buf *bp;

	bp = TAILQ_FIRST(&disksort->bq_head);

	if (bp != NULL && remove)
		TAILQ_REMOVE(&disksort->bq_head, bp, b_actq);

	return (bp);
}

/*
 * Seek sort for disks.
 *
 * There are two queues.  The first queue holds read requests; the second
 * holds write requests.  The read queue is first-come first-served; the
 * write queue is sorted in ascendening block order.
 * The read queue is processed first.  After PRIO_READ_BURST consecutive
 * read requests with non-empty write queue PRIO_WRITE_REQ requests from
 * the write queue will be processed.
 */
static void
bufq_prio_put(bufq, bp)
	struct bufq_state *bufq;
	struct buf *bp;
{
	struct bufq_prio *prio = bufq->bq_private;
	struct buf *bq;
	int sortby;

	sortby = bufq->bq_flags & BUFQ_SORT_MASK;

	/*
	 * If it's a read request append it to the list.
	 */
	if ((bp->b_flags & B_READ) == B_READ) {
		TAILQ_INSERT_TAIL(&prio->bq_read, bp, b_actq);
		return;
	}

	bq = TAILQ_FIRST(&prio->bq_write);

	/*
	 * If the write list is empty, simply append it to the list.
	 */
	if (bq == NULL) {
		TAILQ_INSERT_TAIL(&prio->bq_write, bp, b_actq);
		prio->bq_write_next = bp;
		return;
	}

	/*
	 * If we lie after the next request, insert after this request.
	 */
	if (buf_inorder(prio->bq_write_next, bp, sortby))
		bq = prio->bq_write_next;

	/*
	 * Search for the first request at a larger block number.
	 * We go before this request if it exists.
	 */
	while (bq != NULL && buf_inorder(bq, bp, sortby))
		bq = TAILQ_NEXT(bq, b_actq);

	if (bq != NULL) {
		TAILQ_INSERT_BEFORE(bq, bp, b_actq);
	} else {
		TAILQ_INSERT_TAIL(&prio->bq_write, bp, b_actq);
	}
}

static struct buf *
bufq_prio_get(bufq, remove)
	struct bufq_state *bufq;
	int remove;
{
	struct bufq_prio *prio = bufq->bq_private;
	struct buf *bp;

	/*
	 * If no current request, get next from the lists.
	 */
	if (prio->bq_next == NULL) {
		/*
		 * If at least one list is empty, select the other.
		 */
		if (TAILQ_FIRST(&prio->bq_read) == NULL) {
			prio->bq_next = prio->bq_write_next;
			prio->bq_read_burst = 0;
		} else if (prio->bq_write_next == NULL) {
			prio->bq_next = TAILQ_FIRST(&prio->bq_read);
			prio->bq_read_burst = 0;
		} else {
			/*
			 * Both list have requests.  Select the read list up
			 * to PRIO_READ_BURST times, then select the write
			 * list PRIO_WRITE_REQ times.
			 */
			if (prio->bq_read_burst++ < PRIO_READ_BURST)
				prio->bq_next = TAILQ_FIRST(&prio->bq_read);
			else if (prio->bq_read_burst <
			    PRIO_READ_BURST + PRIO_WRITE_REQ)
				prio->bq_next = prio->bq_write_next;
			else {
				prio->bq_next = TAILQ_FIRST(&prio->bq_read);
				prio->bq_read_burst = 0;
			}
		}
	}

	bp = prio->bq_next;

	if (bp != NULL && remove) {
		if ((bp->b_flags & B_READ) == B_READ) {
			TAILQ_REMOVE(&prio->bq_read, bp, b_actq);
		} else {
			/*
			 * Advance the write pointer before removing
			 * bp since it is actually prio->bq_write_next.
			 */
			prio->bq_write_next =
			    TAILQ_NEXT(prio->bq_write_next, b_actq);
			TAILQ_REMOVE(&prio->bq_write, bp, b_actq);
			if (prio->bq_write_next == NULL)
				prio->bq_write_next =
				    TAILQ_FIRST(&prio->bq_write);
		}

		prio->bq_next = NULL;
	}

	return (bp);
}

/*
 * Cyclical scan (CSCAN)
 */
RB_HEAD(bqhead, buf);
struct cscan_queue {
	struct bqhead 	cq_head[2];			/* actual lists of buffers */
	int 			cq_idx;				/* current list index */
	int 			cq_sortby;			/* BUFQ_SORT_MASK */
	int 			cq_lastcylin;		/* b_cylin of the last request */
	daddr_t 		cq_lastrawblkno;	/* b_rawblkno of the last request */
};

static int __inline cscan_empty(const struct cscan_queue *);
static void cscan_put(struct cscan_queue *, struct buf *, int);
static struct buf *cscan_get(struct cscan_queue *, int);
static void cscan_init(struct cscan_queue *, int);

static signed int
buf_cmp(b1, b2, sortby)
	const struct buf *b1, *b2;
	int sortby;
{
	if (buf_inorder(b2, b1, sortby)) {
		return 1; /* b1 > b2 */
	}
	if (buf_inorder(b1, b2, sortby)) {
		return -1; /* b1 < b2 */
	}
	return 0;
}

static signed int
cscan_tree_compare_nodes(context, n1, n2)
	void *context;
	const void *n1, *n2;
{
	const struct cscan_queue * const q = context;
	const struct buf * const b1 = n1;
	const struct buf * const b2 = n2;
	const int sortby = q->cq_sortby;
	const int diff = buf_cmp(b1, b2, sortby);

	if (diff != 0) {
		return (diff);
	}

	if (b1 > b2) {
		return (1);
	}
	if (b1 < b2) {
		return (-1);
	}
	return (0);
}

RB_PROTOTYPE(bqhead, buf, b_rbnode, cscan_tree_compare_nodes);
RB_GENERATE(bqhead, buf, b_rbnode, cscan_tree_compare_nodes);

static __inline int
cscan_empty(q)
	const struct cscan_queue *q;
{
	return (RB_EMPTY(&q->cq_head[0]) && RB_EMPTY(&q->cq_head[1]));
}

static void
cscan_put(q, bp, sortby)
	struct cscan_queue *q;
	struct buf *bp;
	int sortby;
{
	struct buf tmp;
	struct buf *it;
	struct bqhead *bqh;
	int idx;

	tmp.b_cylin = q->cq_lastcylin;
	tmp.b_rawblkno = q->cq_lastrawblkno;

	if (buf_inorder(bp, &tmp, sortby))
		idx = 1 - q->cq_idx;
	else
		idx = q->cq_idx;

	bqh = &q->cq_head[idx];

	RB_FOREACH(&it, bqh, buf) {
		if (buf_inorder(bp, it, sortby)) {
			break;
		}
	}

	if (it != NULL) {
		RB_INSERT(buf, bqh, it);
	} else {
		RB_INSERT(buf, bqh, bp);
	}
}

static struct buf *
cscan_get(q, remove)
	struct cscan_queue *q;
	int remove;
{
	int idx = q->cq_idx;
	struct bqhead *bqh;
	struct buf *bp;

	bqh = &q->cq_head[idx];
	bp = RB_FIRST(buf, bqh);

	if (bp == NULL) {
		/* switch queue */
		idx = 1 - idx;
		bqh = &q->cq_head[idx];
		bp = RB_FIRST(buf, bqh);
	}

	KDASSERT((bp != NULL && !cscan_empty(q)) || (bp == NULL && cscan_empty(q)));

	if (bp != NULL && remove) {
		q->cq_idx = idx;
		RB_REMOVE(buf, bqh, bp);

		q->cq_lastcylin = bp->b_cylin;
		q->cq_lastrawblkno =
		    bp->b_rawblkno + (bp->b_bcount >> DEV_BSHIFT);
	}

	return (bp);
}

static void
cscan_init(q, sortby)
	struct cscan_queue *q;
	int sortby;
{
	q->cq_sortby = sortby;
	RB_INIT(&q->cq_head[0]);
	RB_INIT(&q->cq_head[1]);
}

/*
 * Per-prioritiy CSCAN.
 *
 * XXX probably we should have a way to raise
 * priority of the on-queue requests.
 */
#define	PRIOCSCAN_NQUEUE	3

struct priocscan_queue {
	struct cscan_queue 	q_queue;
	int 				q_burst;
};

struct bufq_priocscan {
	struct priocscan_queue bq_queue[PRIOCSCAN_NQUEUE];

#if 0
	/*
	 * XXX using "global" head position can reduce positioning time
	 * when switching between queues.
	 * although it might affect against fairness.
	 */
	daddr_t bq_lastrawblkno;
	int bq_lastcylinder;
#endif
};

/*
 * how many requests to serve when having pending requests on other queues.
 *
 * XXX tune
 */
const int priocscan_burst[] = {
	64, 16, 4
};

static void bufq_priocscan_put(struct bufq_state *, struct buf *);
static struct buf *bufq_priocscan_get(struct bufq_state *, int);
static void bufq_priocscan_init(struct bufq_state *);
static __inline struct cscan_queue *bufq_priocscan_selectqueue(struct bufq_priocscan *, const struct buf *);

static __inline struct cscan_queue *
bufq_priocscan_selectqueue(struct bufq_priocscan *q, const struct buf *bp)
{
	static const int priocscan_priomap[] = {
		[BPRIO_TIMENONCRITICAL] = 2,
		[BPRIO_TIMELIMITED] = 1,
		[BPRIO_TIMECRITICAL] = 0
	};

	return &q->bq_queue[priocscan_priomap[BIO_GETPRIO(bp)]].q_queue;
}

static void
bufq_priocscan_put(struct bufq_state *bufq, struct buf *bp)
{
	struct bufq_priocscan *q = bufq->bq_private;
	struct cscan_queue *cq;
	const int sortby = bufq->bq_flags & BUFQ_SORT_MASK;

	cq = bufq_priocscan_selectqueue(q, bp);
	cscan_put(cq, bp, sortby);
}

static struct buf *
bufq_priocscan_get(bufq, remove)
	struct bufq_state *bufq;
	int remove;
{
	struct bufq_priocscan *q = bufq->bq_private;
	struct priocscan_queue *pq, *npq;
	struct priocscan_queue *first; /* first non-empty queue */
	const struct priocscan_queue *epq;
	const struct cscan_queue *cq;
	struct buf *bp;
	bool_t single; /* true if there's only one non-empty queue */

	pq = &q->bq_queue[0];
	epq = pq + PRIOCSCAN_NQUEUE;
	for (; pq < epq; pq++) {
		cq = &pq->q_queue;
		if (!cscan_empty(cq))
			break;
	}
	if (pq == epq) {
		/* there's no requests */
		return NULL;
	}

	first = pq;
	single = TRUE;
	for (npq = first + 1; npq < epq; npq++) {
		cq = &npq->q_queue;
		if (!cscan_empty(cq)) {
			single = FALSE;
			if (pq->q_burst > 0)
				break;
			pq = npq;
		}
	}
	if (single) {
		/*
		 * there's only a non-empty queue.  just serve it.
		 */
		pq = first;
	} else if (pq->q_burst > 0) {
		/*
		 * XXX account only by number of requests.  is it good enough?
		 */
		pq->q_burst--;
	} else {
		/*
		 * no queue was selected due to burst counts
		 */
		int i;
#ifdef DEBUG
		for (i = 0; i < PRIOCSCAN_NQUEUE; i++) {
			pq = &q->bq_queue[i];
			cq = &pq->q_queue;
			if (!cscan_empty(cq) && pq->q_burst)
				panic("%s: inconsist", __func__);
		}
#endif /* DEBUG */

		/*
		 * reset burst counts
		 */
		for (i = 0; i < PRIOCSCAN_NQUEUE; i++) {
			pq = &q->bq_queue[i];
			pq->q_burst = priocscan_burst[i];
		}

		/*
		 * serve first non-empty queue.
		 */
		pq = first;
	}

	KDASSERT(!cscan_empty(&pq->q_queue));
	bp = cscan_get(&pq->q_queue, remove);
	KDASSERT(bp != NULL);
	KDASSERT(&pq->q_queue == bufq_priocscan_selectqueue(q, bp));

	return bp;
}

static void
bufq_priocscan_init(bufq)
	struct bufq_state *bufq;
{
	struct bufq_priocscan *q;
	const int sortby;
	int i;

	sortby = bufq->bq_flags & BUFQ_SORT_MASK;
	bufq->bq_get = bufq_priocscan_get;
	bufq->bq_put = bufq_priocscan_put;
	bufq->bq_private = malloc(sizeof(struct bufq_priocscan),
	    M_DEVBUF, M_ZERO);

	q = bufq->bq_private;
	for (i = 0; i < PRIOCSCAN_NQUEUE; i++) {
		struct cscan_queue *cq = &q->bq_queue[i].q_queue;

		cscan_init(cq, sortby);
	}
}

/*
 * Create a device buffer queue.
 */
void
bufq_alloc(bufq, flags)
	struct bufq_state *bufq;
	int flags;
{
	struct bufq_fcfs *fcfs;
	struct bufq_disksort *disksort;
	struct bufq_prio *prio;

	bufq->bq_flags = flags;

	switch (flags & BUFQ_SORT_MASK) {
	case BUFQ_SORT_RAWBLOCK:
	case BUFQ_SORT_CYLINDER:
		break;
	case 0:
		if ((flags & BUFQ_METHOD_MASK) == BUFQ_FCFS) {
			break;
		}
		/* FALLTHROUGH */
	default:
		panic("bufq_alloc: sort out of range");
	}

	switch (flags & BUFQ_METHOD_MASK) {
	case BUFQ_FCFS:
		bufq->bq_get = bufq_fcfs_get;
		bufq->bq_put = bufq_fcfs_put;
		MALLOC(bufq->bq_private, struct bufq_fcfs *, sizeof(struct bufq_fcfs), M_DEVBUF, M_ZERO);
		fcfs = (struct bufq_fcfs *)bufq->bq_private;
		TAILQ_INIT(&fcfs->bq_head);
		break;
	case BUFQ_DISKSORT:
		bufq->bq_get = bufq_disksort_get;
		bufq->bq_put = bufq_disksort_put;
		MALLOC(bufq->bq_private, struct bufq_disksort *, sizeof(struct bufq_disksort), M_DEVBUF, M_ZERO);
		disksort = (struct bufq_disksort *)bufq->bq_private;
		TAILQ_INIT(&disksort->bq_head);
		break;
	case BUFQ_READ_PRIO:
		bufq->bq_get = bufq_prio_get;
		bufq->bq_put = bufq_prio_put;
		MALLOC(bufq->bq_private, struct bufq_prio *, sizeof(struct bufq_prio), M_DEVBUF, M_ZERO);
		prio = (struct bufq_prio *)bufq->bq_private;
		TAILQ_INIT(&prio->bq_read);
		TAILQ_INIT(&prio->bq_write);
		break;
	case BUFQ_PRIOCSCAN:
		bufq_priocscan_init(bufq);
		break;
	default:
		panic("bufq_alloc: method out of range");
	}
}

/*
 * Destroy a device buffer queue.
 */
void
bufq_free(bufq)
	struct bufq_state *bufq;
{
	KASSERT(bufq->bq_private != NULL);
	KASSERT(BUFQ_PEEK(bufq) == NULL);

	FREE(bufq->bq_private, M_DEVBUF);
	bufq->bq_get = NULL;
	bufq->bq_put = NULL;
}
