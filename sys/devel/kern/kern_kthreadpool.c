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
#include <devel/sys/threadpool.h>
#include <devel/sys/kthread.h>
#include <sys/stdbool.h>
#include <sys/percpu.h>

#include <vm/include/vm_param.h>

#include <devel/sys/malloctypes.h>

#include <machine/cpuinfo.h>

struct kthreadpool_thread 				ktpool_thread;
struct lock_object						*kthreadpools_lock;

struct kthreadpool_unbound {
	struct kthreadpool					ktpu_pool;

	/* protected by kthreadpools_lock */
	LIST_ENTRY(kthreadpool_unbound)		ktpu_link;
	uint64_t							ktpu_refcnt;
};
static LIST_HEAD(, kthreadpool_unbound) unbound_kthreadpools;

struct kthreadpool_percpu {
	struct percpu						*ktpp_percpu;
	pid_t								ktpp_pri;

	/* protected by threadpools_lock */
	LIST_ENTRY(kthreadpool_percpu)		ktpp_link;
	uint64_t							ktpp_refcnt;
};
static LIST_HEAD(, kthreadpool_percpu) 	percpu_kthreadpools;

static struct kthreadpool_unbound *
kthreadpool_lookup_unbound(pri)
	pid_t pri;
{
	struct kthreadpool_unbound *ktpu;
	LIST_FOREACH(ktpu, &unbound_kthreadpools, ktpu_link) {
		if (ktpu->ktpu_pool.ktp_pri == pri)
			return ktpu;
	}
	return NULL;
}

static void
kthreadpool_insert_unbound(ktpu)
	struct kthreadpool_unbound *ktpu;
{
	KASSERT(kthreadpool_lookup_unbound(ktpu->ktpu_pool.ktp_pri) == NULL);
	LIST_INSERT_HEAD(&unbound_kthreadpools, ktpu, ktpu_link);
}

static void
kthreadpool_remove_unbound(ktpu)
	struct kthreadpool_unbound *ktpu;
{
	KASSERT(kthreadpool_lookup_unbound(ktpu->ktpu_pool.ktp_pri) == ktpu);
	LIST_REMOVE(ktpu, ktpu_link);
}

static struct kthreadpool_percpu *
kthreadpool_lookup_percpu(pri)
	pid_t pri;
{
	struct kthreadpool_percpu *ktpp;

	LIST_FOREACH(ktpp, &percpu_kthreadpools, ktpp_link) {
		if (ktpp->ktpp_pri == pri)
			return ktpp;
	}
	return NULL;
}

static void
kthreadpool_insert_percpu(ktpp)
	struct kthreadpool_percpu *ktpp;
{
	KASSERT(kthreadpool_lookup_percpu(ktpp->ktpp_pri) == NULL);
	LIST_INSERT_HEAD(&percpu_kthreadpools, ktpp, ktpp_link);
}

static void
kthreadpool_remove_percpu(ktpp)
	struct kthreadpool_percpu *ktpp;
{
	KASSERT(kthreadpool_lookup_percpu(ktpp->ktpp_pri) == ktpp);
	LIST_REMOVE(ktpp, ktpp_link);
}

void
kthreadpool_init(void)
{
	&ktpool_thread = (struct kthreadpool_thread *)malloc(sizeof(struct kthreadpool_thread *), M_KTPOOLTHREAD, NULL);
	LIST_INIT(&unbound_kthreadpools);
	LIST_INIT(&percpu_kthreadpools);
	simple_lock_init(&kthreadpools_lock, "kthreadpools_lock");
}

/* KThread pool creation */
static int
kthreadpool_create(ktpool, pri)
	struct kthreadpool *ktpool;
	pid_t pri;
{
	struct proc *p;
	int ktflags;
	int error;

	simple_lock(&ktpool->ktp_lock);
	/* XXX overseer */
	TAILQ_INIT(&ktpool->ktp_jobs);
	TAILQ_INIT(&ktpool->ktp_idle_threads);

	ktpool->ktp_refcnt = 1;
	ktpool->ktp_flags = 0;
	ktpool->ktp_pri = pri;

	ktpool->ktp_overseer.ktpt_kthread = NULL;
	ktpool->ktp_overseer.ktpt_pool = ktpool;
	ktpool->ktp_overseer.ktpt_job = NULL;

	ktflags = 0;
	if (pri) {
		error = kthread_create(&kthreadpool_overseer_thread, &ktpool->ktp_overseer, &p, "kthread pooloverseer/%d@%d");
	}
	if (error) {
		goto fail;
	}

	ktpool->ktp_overseer.ktpt_kthread = p->p_kthreado;

fail:
	KASSERT(error);
	KASSERT(ktpool->ktp_overseer.ktpt_job == NULL);
	KASSERT(ktpool->ktp_overseer.ktpt_pool == ktpool);
	KASSERT(ktpool->ktp_flags == 0);
	KASSERT(ktpool->ktp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&ktpool->ktp_idle_threads));
	KASSERT(TAILQ_EMPTY(&ktpool->ktp_jobs));
	simple_unlock(&ktpool->ktp_lock);

	return (error);
}

/* Thread pool destruction */
static void
kthreadpool_destroy(ktpool)
	struct kthreadpool *ktpool;
{
	struct kthreadpool_thread *kthread;

	simple_lock(&ktpool->ktp_lock);
	KASSERT(TAILQ_EMPTY(&ktpool->ktp_jobs));
	ktpool->ktp_flags |= KTHREADPOOL_DYING;

	KASSERT(ktpool->ktp_overseer.ktpt_job == NULL);
	KASSERT(ktpool->ktp_overseer.ktpt_pool == ktpool);
	KASSERT(ktpool->ktp_flags == KTHREADPOOL_DYING);
	KASSERT(ktpool->ktp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&ktpool->ktp_idle_threads));
	KASSERT(TAILQ_EMPTY(&ktpool->ktp_jobs));

	simple_unlock(&ktpool->ktp_lock);
}

static void
kthreadpool_hold(ktpool)
	struct kthreadpool *ktpool;
{
	ktpool->ktp_refcnt++;
	KASSERT(ktpool->ktp_refcnt != 0);
}

static void
kthreadpool_rele(ktpool)
	struct kthreadpool *ktpool;
{
	KASSERT(0 < ktpool->ktp_refcnt);
}

/* Unbound thread pools */
int
kthreadpool_get(ktpoolp, pri)
	struct kthreadpool **ktpoolp;
	pid_t pri;
{
	struct kthreadpool_unbound *ktpu, *tmp = NULL;
	int error;

	simple_lock(&kthreadpools_lock);
	ktpu = kthreadpool_lookup_unbound(pri);
	if (ktpu == NULL) {
		error = kthreadpool_create(&tmp->ktpu_pool, pri);
		if (error) {
			FREE(tmp, M_KTPOOLTHREAD);
			return error;
		}
		simple_lock(&kthreadpools_lock);
		ktpu = kthreadpool_lookup_unbound(pri);
		if (ktpu == NULL) {
			ktpu = tmp;
			tmp = NULL;
			kthreadpool_insert_unbound(ktpu);
		}
	}
	KASSERT(ktpu != NULL);
	ktpu->ktpu_refcnt++;
	KASSERT(ktpu->ktpu_refcnt != 0);
	simple_unlock(&kthreadpools_lock);

	if (tmp != NULL) {
		kthreadpool_destroy(&tmp->ktpu_pool);
		FREE(tmp, M_KTPOOLTHREAD);
	}

	KASSERT(ktpu != NULL);
	*ktpoolp = &ktpu->ktpu_pool;
	return (0);
}

void
kthreadpool_put(ktpool, pri)
	struct kthreadpool *ktpool;
	pid_t pri;
{
	struct kthreadpool_unbound *ktpu;

	simple_lock(&kthreadpools_lock);
	KASSERT(ktpu == kthreadpool_lookup_unbound(pri));
	KASSERT(0 < ktpu->ktpu_refcnt);
	if (ktpu->ktpu_refcnt-- == 0) {
		kthreadpool_remove_unbound(ktpu);
	} else {
		ktpu = NULL;
	}
	simple_unlock(&kthreadpools_lock);

	if (ktpu) {
		kthreadpool_destroy(&ktpu->ktpu_pool);
		FREE(ktpu, M_KTPOOLTHREAD);
	}
}

static void
kthreadpool_overseer_thread(arg)
	void *arg;
{
	struct kthreadpool_thread *const overseer = arg;
	struct kthreadpool *const ktpool = overseer->ktpt_pool;
	struct proc *p = NULL;
	int ktflags;
	int error;

	/* Wait until we're initialized.  */
	simple_lock(&ktpool->ktp_lock);

	for (;;) {
		/* Wait until there's a job.  */
		while (TAILQ_EMPTY(&ktpool->ktp_jobs)) {
			if (ktpool->ktp_flags & KTHREADPOOL_DYING) {
				break;
			}
		}
		if (__predict_false(TAILQ_EMPTY(&ktpool->ktp_jobs))) {
			break;
		}

		/* If there are no threads, we'll have to try to start one.  */
		if (TAILQ_EMPTY(&ktpool->ktp_idle_threads)) {
			kthreadpool_hold(ktpool);
			simple_unlock(&ktpool->ktp_lock);

			struct kthreadpool_thread *const kthread = (struct kthreadpool_thread *) malloc(sizeof(struct kthreadpool_thread *), M_KTPOOLTHREAD, M_WAITOK);
			kthread->ktpt_proc = NULL;
			kthread->ktpt_pool = ktpool;
			kthread->ktpt_job = NULL;

			ktflags = 0;
			ktflags |= KTHREAD_MPSAFE;
			if (ktpool->ktp_pri < PUSER)
				ktflags |= KTHREAD_TS;
			error = kthread_create(&kthreadpool_thread, kthread, &p, "kthread poolthread/%d@%d");

			simple_lock(&ktpool->ktp_lock);
			if (error) {
				kthreadpool_rele(ktpool);
				continue;
			}
			/*
			 * New kthread now owns the reference to the pool
			 * taken above.
			 */
			KASSERT(p != NULL);
			TAILQ_INSERT_TAIL(&ktpool->ktp_idle_threads, kthread, ktpt_entry);
			kthread->ktpt_proc = p;
			p = NULL;
			continue;
		}

		/* There are idle threads, so try giving one a job.  */
		struct threadpool_job *const job = TAILQ_FIRST(&ktpool->ktp_jobs);

		TAILQ_REMOVE(&ktpool->ktp_jobs, job, job_entry);

		/*
		 * Take an extra reference on the job temporarily so that
		 * it won't disappear on us while we have both locks dropped.
		 */
		threadpool_job_hold(job);
		simple_unlock(&ktpool->ktp_lock);

		simple_lock(job->job_lock);
		/* If the job was cancelled, we'll no longer be its thread. */
		if (__predict_true(job->job_thread == overseer)) {
			simple_lock(&ktpool->ktp_lock);
			if (__predict_false(TAILQ_EMPTY(&ktpool->ktp_idle_threads))) {
				/*
				 * Someone else snagged the thread
				 * first.  We'll have to try again.
				 */

				TAILQ_INSERT_HEAD(&ktpool->ktp_jobs, job, job_entry);

			} else {
				/*
				 * Assign the job to the thread and
				 * wake the thread so it starts work.
				 */
				struct kthreadpool_thread *const thread = TAILQ_FIRST(&ktpool->ktp_idle_threads);

				KASSERT(thread->ktpt_job == NULL);
				TAILQ_REMOVE(&ktpool->ktp_idle_threads, thread, ktpt_entry);
				thread->ktpt_job = job;
				job->job_thread = thread;
			}
			simple_unlock(&ktpool->ktp_lock);
		}
		threadpool_job_rele(job);
		simple_unlock(job->job_lock);

		simple_lock(&ktpool->ktp_lock);
	}
	kthreadpool_rele(ktpool);
	simple_unlock(&ktpool->ktp_lock);

	kthread_exit(0);
}

static void
kthreadpool_thread(arg)
	void *arg;
{
	struct kthreadpool_thread *const kthread;
	struct kthreadpool *const ktpool;

	kthread = arg;
	ktpool = kthread->ktpt_pool;

	/* Wait until we're initialized and on the queue.  */
	simple_lock(&ktpool->ktp_lock);

	KASSERT(kthread->ktpt_proc == curproc);
	for (;;) {
		/* Wait until we are assigned a job.  */
		while (kthread->ktpt_job == NULL) {

		}
		if (__predict_false(kthread->ktpt_job == NULL)) {
			TAILQ_REMOVE(&ktpool->ktp_idle_threads, kthread, ktpt_entry);
			break;
		}
		struct threadpool_job *const job = kthread->ktpt_job;
		KASSERT(job != NULL);

		/* Set our proc name to reflect what job we're doing.  */
		PROC_LOCK(curproc);
		char *const proc_name = curproc->p_name;
		kthread->ktpt_kthread_savedname = curproc->p_name;
		curproc->p_name = job->job_name;
		PROC_UNLOCK(curproc);

		simple_unlock(&ktpool->ktp_lock);

		/* Run the job.  */
		(*job->job_func)(job);

		/* proc name restored in threadpool_job_done(). */
		KASSERTMSG((curproc->p_name == proc_name), "someone forgot to call threadpool_job_done()!");

		/*
		 * We can compare pointers, but we can no longer deference
		 * job after this because threadpool_job_done() drops the
		 * last reference on the job while the job is locked.
		 */

		simple_lock(&ktpool->ktp_lock);
		KASSERT(kthread->ktpt_job == job);
		kthread->ktpt_job = NULL;
		TAILQ_INSERT_TAIL(&ktpool->ktp_idle_threads, kthread, ktpt_entry);
	}
	kthreadpool_rele(ktpool);
	simple_unlock(&ktpool->ktp_lock);

	kthread_exit(0);
}
/*
static bool_t
threadpool_pri_is_valid(pri_t pri)
{
	return (pri == PRI_NONE || (pri >= PRI_USER && pri < PRI_COUNT));
}
*/
/* Percpu threadpools */
int
kthreadpool_percpu_get(ktpool_percpup, pri)
	struct kthreadpool_percpu **ktpool_percpup;
	pid_t pri;
{
	struct kthreadpool_percpu *ktpool_percpu, *tmp = NULL;
	int error;
	ktpool_percpu = kthreadpool_lookup_percpu(pri);
	simple_lock(&kthreadpools_lock);
	if(ktpool_percpu == NULL) {
		simple_unlock(&kthreadpools_lock);
		error = kthreadpool_percpu_create(&tmp, pri);
		if (error) {
			return (error);
		}
		KASSERT(tmp != NULL);
		simple_lock(&kthreadpools_lock);
		ktpool_percpu = kthreadpool_lookup_percpu(pri);
		if (ktpool_percpu == NULL) {
			ktpool_percpu = tmp;
			tmp = NULL;
			kthreadpool_insert_percpu(ktpool_percpu);
		}
	}
	KASSERT(ktpool_percpu != NULL);
	ktpool_percpu->ktpp_refcnt++;
	KASSERT(ktpool_percpu->ktpp_refcnt != 0);
	simple_unlock(&kthreadpools_lock);

	if (tmp != NULL) {
		kthreadpool_percpu_destroy(tmp);
	}
	KASSERT(ktpool_percpu != NULL);
	*ktpool_percpup = ktpool_percpu;
	return (0);
}

void
kthreadpool_percpu_put(ktpool_percpu, pri)
	struct kthreadpool_percpu *ktpool_percpu;
	pid_t pri;
{
	//KASSERT(kthreadpool_pri_is_valid(pri));

	simple_lock(&kthreadpools_lock);
	KASSERT(ktpool_percpu == kthreadpool_lookup_percpu(pri));
	KASSERT(0 < ktpool_percpu->ktpp_refcnt);
	if (--ktpool_percpu->ktpp_refcnt == 0) {
		kthreadpool_remove_percpu(ktpool_percpu);
	} else {
		ktpool_percpu = NULL;
	}
	simple_unlock(&kthreadpools_lock);

	if (ktpool_percpu) {
		kthreadpool_percpu_destroy(ktpool_percpu);
	}
}

static int
kthreadpool_percpu_create(ktpool_percpup, pri)
	struct kthreadpool_percpu **ktpool_percpup;
	pid_t pri;
{
	struct kthreadpool_percpu *ktpool_percpu;
	int error, cpu;

	for (cpu = 0; cpu < cpu_number(); cpu++) {
		error = kthreadpool_percpu_start(ktpool_percpu, pri, cpu);
	}

	*ktpool_percpup = (struct kthreadpool_percpu *)ktpool_percpu;
	return (error);
}

int
kthreadpool_percpu_start(ktpool_percpu, pri, ncpus)
	struct kthreadpool_percpu *ktpool_percpu;
	pid_t pri;
	int ncpus;
{
	ktpool_percpu = (struct kthreadpool_percpu *) malloc(
			sizeof(struct kthreadpool_percpu *), M_KTPOOLTHREAD, M_WAITOK);
	ktpool_percpu->ktpp_pri = pri;
	ktpool_percpu->ktpp_percpu = percpu_create(&cpu_info, sizeof(struct kthreadpool*), 1, ncpus);

	if (ktpool_percpu->ktpp_percpu != NULL) {
		/* Success!  */
		kthreadpool_percpu_init(pri);
		return (0);
	}

	kthreadpool_percpu_fini();
	percpu_extent_free(ktpool_percpu->ktpp_percpu,
			ktpool_percpu->ktpp_percpu->pc_start,
			ktpool_percpu->ktpp_percpu->pc_end);
	free(ktpool_percpu, M_KTPOOLTHREAD);
	return (ENOMEM);
}

static void
kthreadpool_percpu_destroy(ktpool_percpu)
	struct kthreadpool_percpu *ktpool_percpu;
{
	percpu_free(ktpool_percpu->ktpp_percpu, sizeof(struct kthreadpool *));
	free(ktpool_percpu, M_KTPOOLTHREAD);
}

static void
kthreadpool_percpu_init(pri)
	u_char pri;
{
	struct kthreadpool **const ktpoolp;
	int error;

	*ktpoolp = (struct kthreadpool *) malloc(sizeof(struct kthreadpool *), M_KTPOOLTHREAD, M_WAITOK);
	error = kthreadpool_create(*ktpoolp, pri);
	if (error) {
		KASSERT(error == ENOMEM);
		free(*ktpoolp, M_KTPOOLTHREAD);
		*ktpoolp = NULL;
	}
}

static void
kthreadpool_percpu_fini(void)
{
	struct kthreadpool **const ktpoolp;

	if (*ktpoolp == NULL) {	/* initialization failed */
		return;
	}
	kthreadpool_destroy(*ktpoolp);
	free(*ktpoolp, M_KTPOOLTHREAD);
}

/* KThreadpool Jobs */
void
kthreadpool_job_init(struct threadpool_job *job, threadpool_job_fn_t func, struct lock *lock, char *name, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void) snprintf(job->job_name, sizeof(job->job_name), fmt, ap);
	va_end(ap);

	threadpool_job_init(job, func, lock, name);
}

void
kthreadpool_job_destroy(job)
	struct threadpool_job *job;
{
	struct kthreadpool_thread *kthread;

	kthread = job->job_thread;
	threadpool_job_destroy(kthread, job);
	job->job_thread = kthread;
}

void
kthreadpool_job_done(job)
	struct threadpool_job *job;
{
	struct kthreadpool_thread *kthread;

	kthread = job->job_thread;
	KASSERT(kthread != NULL);
	KASSERT(kthread->ktpt_proc == curproc);
	/*
	 * We can safely read this field; it's only modified right before
	 * we call the job work function, and we are only preserving it
	 * to use here; no one cares if it contains junk afterward.
	 */
	PROC_LOCK(curproc);
	curproc->p_name = kthread->ktpt_kthread_savedname;
	PROC_UNLOCK(curproc);

	threadpool_job_done(job);
	kthread = NULL;
	job->job_thread = kthread;
}

void
kthreadpool_schedule_job(ktpool, job)
	struct kthreadpool *ktpool;
	struct threadpool_job *job;
{
	struct kthreadpool_thread *kthread;

	kthread = job->job_thread;
	if (__predict_true(kthread != NULL)) {
		return;
	}
	if (__predict_false(TAILQ_EMPTY(&ktpool->ktp_idle_threads))) {
		kthread = &ktpool->ktp_overseer;
		threadpool_job_schedule(&ktpool->ktp_jobs, job, &ktpool->ktp_lock, TPJ_TAIL);
	}  else {
		/* Assign it to the first idle thread.  */
		kthread = TAILQ_FIRST(&ktpool->ktp_idle_threads);
		kthread->ktpt_job = job;
	}
	job->job_thread = kthread;
}

bool_t
kthreadpool_cancel_job_async(ktpool, job)
	struct kthreadpool 		*ktpool;
	struct threadpool_job 	*job;
{

	struct kthreadpool_thread *kthread;

	kthread = job->job_thread;

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
	if (kthread == NULL) {
		/* Nothing to do.  Guaranteed not running.  */
		return (TRUE);
	} else if (kthread == &ktpool->ktp_overseer) {
		/* Take it off the list to guarantee it won't run.  */
		kthread = NULL;
		threadpool_job_cancel(&ktpool->ktp_jobs, job, &ktpool->ktp_lock);
		job->job_thread = kthread;
		return (TRUE);
	} else {
		return (FALSE);
	}
}

void
kthreadpool_cancel_job(ktpool, job)
	struct kthreadpool *ktpool;
	struct threadpool_job *job;
{
	/*
	 * We may sleep here, but we can't ASSERT_SLEEPABLE() because
	 * the job lock (used to interlock the cv_wait()) may in fact
	 * legitimately be a spin lock, so the assertion would fire
	 * as a false-positive.
	 */

	if (kthreadpool_cancel_job_async(ktpool, job)) {
		return;
	}
}
