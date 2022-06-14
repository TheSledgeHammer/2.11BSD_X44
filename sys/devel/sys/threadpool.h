/*	$NetBSD: threadpool.h,v 1.7 2020/04/25 07:23:21 mlelstv Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef SYS_THREADPOOL_H_
#define SYS_THREADPOOL_H_

#include <sys/kthread.h>
#include <libuthread/uthread.h>

/* Threadpool Jobs */
struct kthreadpool;
struct threadpool_job;
struct kthreadpool_percpu;
struct kthreadpool_thread;

struct job_head;
TAILQ_HEAD(job_head, threadpool_job);
typedef void threadpool_job_fn_t(struct threadpool_job *);
struct threadpool_job  {
	TAILQ_ENTRY(threadpool_job)			job_entry;
	threadpool_job_fn_t					*job_func;
	const char							*job_name;
	struct lock							*job_lock;
	volatile unsigned int				job_refcnt;
	struct kthreadpool_thread			*job_kthread;
	struct uthreadpool_thread			*job_uthread;
};

/* threadpool job queue flags */
#define TPJ_HEAD 0	/* add to head of job queue */
#define TPJ_TAIL 1	/* add to tail of job queue */

#define threadpool_lock(lock) 	(simple_lock(lock->lk_lnterlock))
#define threadpool_unlock(lock) (simple_unlock(lock->lk_lnterlock))

void	threadpool_job_init(void *, struct threadpool_job *, threadpool_job_fn_t, struct lock *, const char *);
void	threadpool_job_enqueue(struct job_head *, struct threadpool_job *, struct lock *, int);
void	threadpool_job_dequeue(struct job_head *, struct threadpool_job *, struct lock *);
struct threadpool_job *threadpool_job_search(struct job_head *, void *, void *, struct lock *);
void	threadpool_job_dead(struct threadpool_job *);
void	threadpool_job_destroy(void *, struct threadpool_job *);
void	threadpool_job_hold(struct threadpool_job *);
void	threadpool_job_rele(struct threadpool_job *);
void	threadpool_job_done(struct threadpool_job *);
void	threadpool_job_schedule(struct job_head *, struct threadpool_job *, struct lock *, int);
void	threadpool_job_cancel(struct job_head *, struct threadpool_job *, struct lock *);
#endif /* SYS_THREADPOOL_H_ */
