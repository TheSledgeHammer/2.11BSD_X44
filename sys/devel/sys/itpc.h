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
 */

/*
 * Two Threadpools:
 * - Kernel Thread pool
 * - User Thread pool (userspace)
 *
 * primary jobs:
 * - dispatch and hold x number of threads
 * - send & receive jobs between user & kernel threads when applicable
 *
 * IPC:
 * - Use thread pool/s as the ipc. (bi-directional fifo queue)
 * 	- i.e. all requests go through the associated threadpool.
 * 	- requests sent in order received. (no priorities)
 * 	- retries failed items are removed from queue and added at the end of the queue. Items have 5 attempts.
 * 		- to prevent a lock-ups on failed items
 * 	- confirmation: to prevent thread pools requesting the same task
 * 	- Tasks added to work queue
 * 	- Only jobs flagged for sending
 *
 * 	Look at kthreads & uthreads for corresponding information
 */
#ifndef _SYS_ITPC_H_
#define _SYS_ITPC_H_

#include <sys/queue.h>
#include <devel/sys/threadpool.h>

/* Inter-Threadpool Communication (ITPC) */

struct itpc_list;
TAILQ_HEAD(itpc_list, itpc);
struct itpc {
	TAILQ_ENTRY(itpc) 		itpc_entry;
	union {
		struct kthread		*i_kthread;
		struct uthread  	*i_uthread;
	} ithread;

	union {
		struct kthreadpool 	*i_kthreadpool;
		struct uthreadpool 	*i_uthreadpool;
	} ithreadpool;
};

extern struct itpc_list itpc_header;

/* kthreads & kthreadpools */
#define IKTHREAD(itpc)					((itpc)->ithread.i_kthread)
#define IKTHREADPOOL(itpc)				((itpc)->ithreadpool.i_kthreadpool)
#define SET_IKTHREAD(itpc, kt)			(IKTHREAD(itpc) = (kt))
#define SET_IKTHREADPOOL(itpc, ktpool)	(IKTHREADPOOL(itpc) = (ktpool))
#define GET_IKTHREAD(itpc)				(IKTHREAD(itpc))
#define GET_IKTHREADPOOL(itpc)			(IKTHREADPOOL(itpc))

/* uthreads & uthreadpools */
#define IUTHREAD(itpc)					((itpc)->ithread.i_uthread)
#define IUTHREADPOOL(itpc)				((itpc)->ithreadpool.i_uthreadpool)
#define SET_IUTHREAD(itpc, ut)			(IUTHREAD(itpc) = (ut))
#define SET_IUTHREADPOOL(itpc, utpool)	(IUTHREADPOOL(itpc) = (utpool))
#define GET_IUTHREAD(itpc)				(IUTHREAD(itpc))
#define GET_IUTHREADPOOL(itpc)			(IUTHREADPOOL(itpc))

void threadpool_job_init(void *, struct threadpool_job *, threadpool_job_fn_t, struct lock *, char *, const char *, ...);
void threadpool_job_dead(struct threadpool_job *);
void threadpool_job_destroy(struct threadpool_job *);
void threadpool_job_hold(struct threadpool_job *);
void threadpool_job_rele(struct threadpool_job *);
void threadpool_job_done(struct threadpool_job *);

/* older version */
TAILQ_HEAD(itpc_head, threadpool_itpc);
struct threadpool_itpc {
	struct itpc_head					itpc_header;		/* Threadpool ITPC queue header */
	TAILQ_ENTRY(threadpool_itpc) 		itpc_entry;			/* Threadpool ITPC queue entries */
	int 								itpc_refcnt;		/* Threadpool ITPC entries in pool */

	struct kthreadpool					itpc_ktpool;		/* Pointer to Kernel Threadpool */
	struct uthreadpool					itpc_utpool;		/* Pointer to User Threadpool */

	/* job related info */
	struct threadpool_job				itpc_jobs;
	const char							*itpc_job_name;

	/* thread info */
	union  {
		struct {
			struct kthread				*itpc_kthread;
			pid_t						itpc_ktid;			/* KThread's Thread ID */
			struct tgrp 				itpc_ktgrp; 		/* KThread's Thread Group */
			struct job_head				itpc_ktjob;			/* KThread's Job */
		} kt;

		struct {
			struct uthread				*itpc_uthread;
			pid_t						itpc_utid;			/* UThread's Thread ID */
			struct tgrp 				itpc_utgrp; 		/* UThread's Thread Group */
			struct job_head				itpc_utjob;			/* UThread's Job */
		} ut;
	} info;
#define itpc_ktinfo						info.kt
#define itpc_utinfo						info.ut
};

extern struct itpc_threadpool itpc;

/* Threadpool ITPC Job Commands */
#define ITPC_INIT			0
#define ITPC_DEAD			1
#define ITPC_DESTROY		2
#define ITPC_HOLD			3
#define ITPC_RELE			4
#define ITPC_SCHEDULE		5
#define ITPC_DONE			6
#define ITPC_CANCEL			7
#define ITPC_CANCEL_ASYNC	8

/* ITPC job-pools interconnect */
void 	itpc_threadpool_init(void);
void	itpc_job_init(struct itpc_threadpool *, struct threadpool_job *, const char *, va_list);
void	itpc_job_dead(struct itpc_threadpool *, struct threadpool_job *);
void	itpc_job_destroy(struct itpc_threadpool *, struct threadpool_job *);
void 	itpc_job_hold(struct itpc_threadpool *, struct threadpool_job *);
void 	itpc_job_rele(struct itpc_threadpool *, struct threadpool_job *);
void 	itpc_job_done(struct itpc_threadpool *, struct threadpool_job *);

struct kthreadpool	itpc_kthreadpool(struct itpc_threadpool *, struct kthreadpool *);
struct uthreadpool	itpc_uthreadpool(struct itpc_threadpool *,  struct uthreadpool *);

/* kthreadpools */
void	kthreadpool_init(void);
int		kthreadpool_get(struct kthreadpool **, u_char);
void	kthreadpool_put(struct kthreadpool *, u_char);
/*
void	threadpool_job_init(struct threadpool_job *, threadpool_job_fn_t, lock_t, const char *, ...);
void	threadpool_job_destroy(struct threadpool_job *);
*/
void	kthreadpool_job_done(struct threadpool_job *);
void	kthreadpool_schedule_job(struct kthreadpool *, struct threadpool_job *);
void	kthreadpool_cancel_job(struct kthreadpool *, struct threadpool_job *);
bool_t	kthreadpool_cancel_job_async(struct kthreadpool *, struct threadpool_job *);

/* uthreadpools */
void	uthreadpool_init(void);
int		uthreadpool_get(struct uthreadpool **, u_char);
void	uthreadpool_put(struct uthreadpool *, u_char);
/*
void	uthreadpool_job_init(struct threadpool_job *, threadpool_job_fn_t, lock_t, const char *, ...);
void	uthreadpool_job_destroy(struct threadpool_job *);
*/
void	uthreadpool_job_done(struct threadpool_job *);
void	uthreadpool_schedule_job(struct uthreadpool *, struct threadpool_job *);
void	uthreadpool_cancel_job(struct uthreadpool *, struct threadpool_job *);
bool_t	uthreadpool_cancel_job_async(struct uthreadpool *, struct threadpool_job *);

#endif /* _SYS_ITPC_H_ */
