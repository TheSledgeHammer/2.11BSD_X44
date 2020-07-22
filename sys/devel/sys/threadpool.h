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

#include "kthread.h"
#include "uthread.h"

/*
 * Two Threadpools:
 * - Kernel Thread pool
 * - User Thread pool
 *
 * primary jobs:
 * - dispatch and hold x number of threads
 * - send & receive jobs between user & kernel threads when applicable
 *
 *
 * IPC:
 * - Use thread pool/s as the ipc. (bi-directional fifo queue)
 * 	- i.e. all requests go through the associated thread pool.
 * 	- requests sent in order received. (no priorities)
 * 	- confirmation: to prevent thread pools requesting the same task
 * 	- Tasks added to work queue
 * 	- Only jobs flagged for sending
 * 	-
 *
 * 	NetBSD:
 * 	- Threadpool_Thread: threads as they relate to lwp
 * 	- Threadpool: Actual Pool of Threads
 * 	- Threadpool_Job: Jobs to be done by threads in threadpool
 *
 * 	Look at kthreadpool & uthreadpool for corresponding information
 */

/* Threadpool Jobs */
TAILQ_HEAD(job_head, threadpool_job);
typedef void threadpool_job_fn_t(struct threadpool_job *);
struct threadpool_job {
	mutex_t								*job_lock;
	TAILQ_ENTRY(threadpool_job)			job_entry;
	volatile unsigned int				job_refcnt;
	threadpool_job_fn_t					*job_fn;
	char								job_name[MAXCOMLEN];

	struct threadpool_itpc				*job_itc;
#define job_ktpool						job_itc->itc_ktpool
#define job_utpool						job_itc->itc_utpool
#define	job_ktp_thread					job_ktpool.ktp_overseer
#define	job_utp_thread					job_utpool.utp_overseer
};


/* Inter-Threadpool-Communication (ITPC) */

TAILQ_HEAD(itc_head, threadpool_itpc);
struct threadpool_itpc {
	struct itc_head						itc_header;		/* Threadpool ITPC queue header */
	TAILQ_ENTRY(threadpool_itpc) 		itc_entry;		/* Threadpool ITPC queue entries */
	struct kthreadpool					itc_ktpool;		/* Pointer to Kernel Threadpool */
	struct uthreadpool					itc_utpool;		/* Pointer to User Threadpool */

	struct job_head						itc_job;		/* Thread' Job */
	struct tgrp 						itc_tgrp; 		/* Thread's Thread Group */
	tid_t								itc_tid;		/* Thread's Thread ID */
	int 								itc_refcnt;		/* Current Number of entries in pool */
};
extern struct itc_threadpool itpc;

/* General ITPC */
void itpc_threadpool_init();
void itpc_threadpool_enqueue(struct threadpool_itpc *, tid_t);
void itpc_threadpool_dequeue(struct threadpool_itpc *, tid_t);
void itpc_check(struct threadpool_itpc *, tid_t);
void itpc_verify(struct threadpool_itpc *, tid_t);

#endif /* SYS_THREADPOOL_H_ */
