/*	$NetBSD: kern_threadpool.c,v 1.18 2020/04/25 17:43:23 thorpej Exp $	*/

/*-
 * Copyright (c) 2014, 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell and Jason R. Thorpe.
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/threadpool.h>
#include <sys/kthread.h>

/* Threadpool Jobs */
void
threadpool_job_init(struct threadpool_job *job, threadpool_job_fn_t func, lock_t lock, char *name, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void) vsnprintf(job->job_name, sizeof(job->job_name), fmt, ap);
	va_end(ap);

	job->job_lock = lock;
	job->job_name = name;
	job->job_refcnt = 0;
	job->job_ktp_thread = NULL;
	job->job_utp_thread = NULL;

	job->job_func = func;
}

void
threadpool_job_dead(struct threadpool_job *job)
{
	panic("threadpool job %p ran after destruction", job);
}

void
threadpool_job_destroy(struct threadpool_job *job)
{
	KASSERTMSG((job->job_ktp_thread == NULL), "job %p still running", job);
	job->job_lock = NULL;
	KASSERT(job->job_ktp_thread == NULL);
	KASSERT(job->job_refcnt == 0);
	job->job_func = threadpool_job_dead;
	(void) strlcpy(job->job_name, "deadjob", sizeof(job->job_name));
}

void
threadpool_job_hold(struct threadpool_job *job)
{
	unsigned int refcnt;

	do {
		refcnt = job->job_refcnt;
		KASSERT(refcnt != UINT_MAX);
	} while (atomic_cas_uint(&job->job_refcnt, refcnt, (refcnt + 1)) != refcnt);
}

void
threadpool_job_rele(struct threadpool_job *job)
{
	unsigned int refcnt;

	KASSERT(mutex_owned(job->job_lock));

	do {
		refcnt = job->job_refcnt;
		KASSERT(0 < refcnt);
		if (refcnt == 1) {
			refcnt = atomic_dec_uint_nv(&job->job_refcnt);
			KASSERT(refcnt != UINT_MAX);
			return;
		}
	} while (atomic_cas_uint(&job->job_refcnt, refcnt, (refcnt - 1)) != refcnt);
}

void
threadpool_job_done(struct threadpool_job *job)
{
	KASSERT(mutex_owned(job->job_lock));
	KASSERT(job->job_ktp_thread != NULL);
	KASSERT(job->job_ktp_thread->ktpt_proc == curproc);

	/*
	 * We can safely read this field; it's only modified right before
	 * we call the job work function, and we are only preserving it
	 * to use here; no one cares if it contains junk afterward.
	 */
	//proc_lock(curproc);
	curproc->p_name = job->job_ktp_thread->ktpt_kthread_savedname;
	//proc_unlock(curproc);

	/*
	 * Inline the work of threadpool_job_rele(); the job is already
	 * locked, the most likely scenario (XXXJRT only scenario?) is
	 * that we're dropping the last reference (the one taken in
	 * threadpool_schedule_job()), and we always do the cv_broadcast()
	 * anyway.
	 */
	KASSERT(0 < job->job_refcnt);
	unsigned int refcnt __diagused = atomic_dec_uint_nv(&job->job_refcnt);
	KASSERT(refcnt != UINT_MAX);
	job->job_ktp_thread = NULL;
}

void
threadpool_schedule_job(struct kthreadpool *ktpool, struct threadpool_job *job)
{
	if (__predict_true(job->job_ktp_thread != NULL)) {
		return;
	}

	threadpool_job_hold(job);

	simple_lock(&ktpool->ktp_lock);
	if (__predict_false(TAILQ_EMPTY(&ktpool->ktp_idle_threads))) {
		job->job_ktp_thread = &ktpool->ktp_overseer;
		TAILQ_INSERT_TAIL(&ktpool->ktp_jobs, job, job_entry);
	} else {
		/* Assign it to the first idle thread.  */
		job->job_ktp_thread = TAILQ_FIRST(&ktpool->ktp_idle_threads);
		job->job_ktp_thread->ktpt_job = job;
	}

	/* Notify whomever we gave it to, overseer or idle thread.  */
	KASSERT(job->job_ktp_thread != NULL);
	simple_unlock(&ktpool->ktp_lock);
}

bool
threadpool_cancel_job_async(struct kthreadpool *ktpool, struct threadpool_job *job)
{
	//KASSERT(mutex_owned(job->job_lock));

	/*
	 * XXXJRT This fails (albeit safely) when all of the following
	 * are true:
	 *
	 *	=> "pool" is something other than what the job was
	 *	   scheduled on.  This can legitimately occur if,
	 *	   for example, a job is percpu-scheduled on CPU0
	 *	   and then CPU1 attempts to cancel it without taking
	 *	   a remote pool reference.  (this might happen by
	 *	   "luck of the draw").
	 *
	 *	=> "job" is not yet running, but is assigned to the
	 *	   overseer.
	 *
	 * When this happens, this code makes the determination that
	 * the job is already running.  The failure mode is that the
	 * caller is told the job is running, and thus has to wait.
	 * The overseer will eventually get to it and the job will
	 * proceed as if it had been already running.
	 */

	if (job->job_ktp_thread == NULL) {
		/* Nothing to do.  Guaranteed not running.  */
		return TRUE;
	} else if (job->job_ktp_thread == &ktpool->ktp_overseer) {
		/* Take it off the list to guarantee it won't run.  */
		job->job_ktp_thread = NULL;
		simple_lock(&ktpool->ktp_lock);

		TAILQ_REMOVE(&ktpool->ktp_jobs, job, job_entry);
		simple_unlock(&ktpool->ktp_lock);
		threadpool_job_rele(job);
		return TRUE;
	} else {
		/* Too late -- already running.  */
		return FALSE;
	}
}

void
threadpool_cancel_job(struct kthreadpool *ktpool, struct threadpool_job *job)
{
	/*
	 * We may sleep here, but we can't ASSERT_SLEEPABLE() because
	 * the job lock (used to interlock the cv_wait()) may in fact
	 * legitimately be a spin lock, so the assertion would fire
	 * as a false-positive.
	 */

	//KASSERT(mutex_owned(job->job_lock));

	if (threadpool_cancel_job_async(ktpool, job))
		return;
}

/****************************************************************/
/* Threadpool Percpu */

/* Not Yet Implemented */

/****************************************************************/
/* Threadpool ITPC */

struct threadpool_itpc itpc;

void
itpc_threadpool_init(void)
{
	itpc_threadpool_setup(&itpc);
}

static void
itpc_threadpool_setup(itpc)
	struct threadpool_itpc *itpc;
{
	if(itpc == NULL) {
		MALLOC(itpc, struct threadpool_itpc *, sizeof(struct threadpool_itpc *), M_ITPC, NULL);
	}
	TAILQ_INIT(itpc->itc_header);
	itpc->itc_refcnt = 0;
}

/* Add a thread to the itc queue */
void
itpc_kthreadpool_enqueue(itpc, tid)
	struct threadpool_itpc *itpc;
	pid_t tid;
{
	struct kthreadpool *ktpool;

	/* check kernel threadpool is not null & has a job/task entry to send */
	if(ktpool != NULL && itpc->itc_tid == tid) {
		itpc->itc_ktpool = ktpool;
		ktpool->ktp_initcq = TRUE;
		itpc->itc_refcnt++;
		TAILQ_INSERT_HEAD(itpc->itc_header, itpc, itc_entry);
	}
}

/*
 * Remove a thread from the itc queue:
 * If threadpool entry is not null, search queue for entry & remove
 */
void
itpc_kthreadpool_dequeue(itpc, tid)
	struct threadpool_itpc *itpc;
	pid_t tid;
{
	struct kthreadpool *ktpool;

	if(ktpool != NULL) {
		TAILQ_FOREACH(itpc, itpc->itc_header, itc_entry) {
			if(TAILQ_NEXT(itpc, itc_entry)->itc_ktpool == ktpool) {
				if(itpc->itc_tid == tid) {
					ktpool->ktp_initcq = FALSE;
					itpc->itc_refcnt--;
					TAILQ_REMOVE(itpc->itc_header, itpc, itc_entry);
				}
			}
		}
	}
}

/* Sender checks request from receiver: providing info */
void
itpc_check_kthreadpool(itpc, tid)
	struct threadpool_itpc *itpc;
	pid_t tid;
{
	struct kthreadpool *ktpool = itpc->itc_ktpool;

	if(ktpool->ktp_issender) {
		printf("kernel threadpool to send");
		if(itpc->itc_tid == tid) {
			printf("kernel tid be found");
			/* check */
		} else {
			if(itpc->itc_tid != tid) {
				if(ktpool->ktp_retcnt <= 5) { /* retry up to 5 times */
					if(ktpool->ktp_initcq) {
						/* exit and re-enter queue increasing retry count */
						itpc_threadpool_dequeue(itpc);
						ktpool->ktp_retcnt++;
						itpc_threadpool_enqueue(itpc);
					}
				} else {
					/* exit queue, reset count to 0 and panic */
					itpc_threadpool_dequeue(itpc);
					ktpool->ktp_retcnt = 0;
					panic("kthreadpool x exited itc queue after 5 failed attempts");
				}
			}
		}
	}
}

/* Receiver verifies request to sender: providing info */
void
itpc_verify_kthreadpool(itpc, tid)
	struct threadpool_itpc *itpc;
	pid_t tid;
{
	struct kthreadpool *ktpool = itpc->itc_ktpool;

	if(ktpool->ktp_isreciever) {
		printf("kernel threadpool to recieve");
		if(itpc->itc_tid == tid) {
			printf("kernel tid found");

		} else {
			printf("kernel tid couldn't be found");
		}
	}
}
