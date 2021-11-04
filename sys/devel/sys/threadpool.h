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

#ifndef SYS_THREADPOOL_H_
#define SYS_THREADPOOL_H_

#include <sys/kthread.h>
#include <libuthread/uthread.h>

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

/* Threadpool Jobs */
TAILQ_HEAD(job_head, threadpool_job);
typedef void threadpool_job_fn_t(struct threadpool_job *);
/* Threadpool Jobs */
struct threadpool_job  {
	TAILQ_ENTRY(threadpool_job)			job_entry;
	struct wqueue						job_wqueue;
	threadpool_job_fn_t					*job_func;
	const char							*job_name;
	lock_t								*job_lock;
	volatile unsigned int				job_refcnt;

	struct threadpool_itpc				*job_itc;
#define job_ktpool						job_itc->itc_ktpool
#define job_utpool						job_itc->itc_utpool
#define	job_ktp_thread					job_ktpool.ktp_overseer
#define	job_utp_thread					job_utpool.utp_overseer
};


/* Inter-Threadpool-Communication (ITPC) */
TAILQ_HEAD(itc_head, threadpool_itpc);
struct threadpool_itpc {
	struct itc_head						itc_header;			/* Threadpool ITPC queue header */
	TAILQ_ENTRY(threadpool_itpc) 		itc_entry;			/* Threadpool ITPC queue entries */
	int 								itc_refcnt;			/* Threadpool ITPC entries in pool */

	struct kthreadpool					itc_ktpool;			/* Pointer to Kernel Threadpool */
	struct uthreadpool					itc_utpool;			/* Pointer to User Threadpool */

	/* job related info */
	struct threadpool_job				itc_jobs;
	const char							*itc_job_name;

	/* thread info */
	union  {
		struct {
			struct kthread				*itc_kthread;
			pid_t						itc_ktid;			/* KThread's Thread ID */
			struct tgrp 				itc_ktgrp; 			/* KThread's Thread Group */
			struct job_head				itc_ktjob;			/* KThread's Job */
		} kt;

		struct {
			struct uthread				*itc_uthread;
			pid_t						itc_utid;			/* UThread's Thread ID */
			struct tgrp 				itc_utgrp; 			/* UThread's Thread Group */
			struct job_head				itc_utjob;			/* UThread's Job */
		} ut;
	} info;
#define itc_ktinfo						info.kt
#define itc_utinfo						info.ut
};

extern struct itc_threadpool itpc;

/* Threadpool ITPC Commands */
#define ITPC_SCHEDULE 	0
#define ITPC_CANCEL		1
#define ITPC_DESTROY	2
#define ITPC_DONE		3

/* General ITPC */
void 	itpc_threadpool_init(void);

/* kthreadpools */
void	kthreadpool_init(void);
int		kthreadpool_get(struct kthreadpool **, u_char);
void	kthreadpool_put(struct kthreadpool *, u_char);

void	threadpool_job_init(struct threadpool_job *, threadpool_job_fn_t, lock_t, const char *, ...);
void	threadpool_job_destroy(struct threadpool_job *);
void	threadpool_job_done(struct threadpool_job *);

void	threadpool_schedule_job(struct kthreadpool *, struct threadpool_job *);
void	threadpool_cancel_job(struct kthreadpool *, struct threadpool_job *);
bool	threadpool_cancel_job_async(struct kthreadpool *, struct threadpool_job *);

/* uthreadpools */
void	uthreadpool_init(void);
int		uthreadpool_get(struct uthreadpool **, u_char);
void	uthreadpool_put(struct uthreadpool *, u_char);

void	uthreadpool_job_init(struct threadpool_job *, threadpool_job_fn_t, lock_t, const char *, ...);
void	uthreadpool_job_destroy(struct threadpool_job *);
void	uthreadpool_job_done(struct threadpool_job *);

void	uthreadpool_schedule_job(struct uthreadpool *, struct threadpool_job *);
void	uthreadpool_cancel_job(struct uthreadpool *, struct threadpool_job *);
bool	uthreadpool_cancel_job_async(struct uthreadpool *, struct threadpool_job *);

#endif /* SYS_THREADPOOL_H_ */
