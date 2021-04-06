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

#include <vm/include/vm_param.h>

/* Threadpool Jobs */
void
threadpool_job_init(struct threadpool_job *job, threadpool_job_fn_t func, lock_t lock, char *name, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void) snprintf(job->job_name, sizeof(job->job_name), fmt, ap);
	va_end(ap);

	job->job_lock = lock;
	job->job_name = name;
	job->job_refcnt = 0;
	job->job_itc->itc_ktpool = NULL;
	job->job_itc->itc_utpool = NULL;
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

	do {
		refcnt = job->job_refcnt;
		KASSERT(0 < refcnt);
		if (refcnt == 1) {
			refcnt = atomic_dec_int_nv(&job->job_refcnt);
			KASSERT(refcnt != UINT_MAX);
			return;
		}
	} while (atomic_cas_uint(&job->job_refcnt, refcnt, (refcnt - 1)) != refcnt);
}

void
threadpool_job_done(struct threadpool_job *job)
{
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
	unsigned int refcnt __diagused = atomic_dec_int_nv(&job->job_refcnt);
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

	if (threadpool_cancel_job_async(ktpool, job))
		return;
}

/****************************************************************/
/* Threadpool Percpu */

/* Not Yet Implemented */

/****************************************************************/

