/*	$NetBSD: kern_threadpool.c,v 1.23 2021/01/23 16:33:49 riastradh Exp $	*/

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
#include <sys/mutex.h>
#include <sys/percpu.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <vm/include/vm_param.h>

#define M_THREADPOOL 	84

TAILQ_HEAD(job_head, threadpool_job);
TAILQ_HEAD(thread_head, threadpool_thread);

struct threadpool_thread {
	struct proc							*tpt_proc;
	char								*tpt_proc_savedname;
	struct threadpool					*tpt_pool;
	struct threadpool_job				*tpt_job;
	TAILQ_ENTRY(threadpool_thread)		tpt_entry;
};

struct threadpool {
	struct lock_object					tp_lock;
	struct threadpool_thread			tp_overseer;
	struct job_head						tp_jobs;
	struct thread_head					tp_idle_threads;
	LIST_ENTRY(threadpool)				tp_link;
	uint64_t							tp_refcnt;
	int									tp_flags;
#define	THREADPOOL_DYING				0x01
	pri_t								tp_pri;
};

struct threadpool_unbound {
	struct threadpool					tpu_pool;

	/* protected by threadpools_lock */
	LIST_ENTRY(threadpool_unbound)		tpu_link;
	uint64_t							tpu_refcnt;
};

struct threadpool_percpu {
	struct percpu 						*tpp_percpu;
	pri_t								tpp_pri;

	/* protected by threadpools_lock */
	LIST_ENTRY(threadpool_percpu)		tpp_link;
	uint64_t							tpp_refcnt;
};

static void	threadpool_job_hold(struct threadpool_job *);
static void	threadpool_job_rele(struct threadpool_job *);

static int	threadpool_percpu_create(struct threadpool_percpu **, pri_t);
static void	threadpool_percpu_destroy(struct threadpool_percpu *);
static void	threadpool_percpu_init(pri_t);
static void	threadpool_percpu_fini(void);

static void	threadpool_overseer_thread(void *);
static void	threadpool_thread(void *);

static void threadpool_wait(struct proc *, const void *, int);

static LIST_HEAD(, threadpool_unbound) 	unbound_threadpools;
static LIST_HEAD(, threadpool_percpu) 	percpu_threadpools;

static struct threadpool_thread 		threadpool_thread_pc;
static struct lock_object				threadpools_lock;

/*
 * Threadpool Unbound
 */
static struct threadpool_unbound *
threadpool_lookup_unbound(pri)
	pri_t pri;
{
	struct threadpool_unbound *tpu;

	LIST_FOREACH(tpu, &unbound_threadpools, tpu_link) {
		if (tpu->tpu_pool.tp_pri == pri)
			return tpu;
	}
	return NULL;
}

static void
threadpool_insert_unbound(tpu)
	struct threadpool_unbound *tpu;
{
	KASSERT(threadpool_lookup_unbound(tpu->tpu_pool.tp_pri) == NULL);
	LIST_INSERT_HEAD(&unbound_threadpools, tpu, tpu_link);
}

static void
threadpool_remove_unbound(tpu)
	struct threadpool_unbound *tpu;
{
	KASSERT(threadpool_lookup_unbound(tpu->tpu_pool.tp_pri) == tpu);
	LIST_REMOVE(tpu, tpu_link);
}

/*
 * Threadpool Percpu
 */
static struct threadpool_percpu *
threadpool_lookup_percpu(pri)
	pri_t pri;
{
	struct threadpool_percpu *tpp;

	LIST_FOREACH(tpp, &percpu_threadpools, tpp_link) {
		if (tpp->tpp_pri == pri)
			return tpp;
	}
	return NULL;
}

static void
threadpool_insert_percpu(tpp)
	struct threadpool_percpu *tpp;
{
	KASSERT(threadpool_lookup_percpu(tpp->tpp_pri) == NULL);
	LIST_INSERT_HEAD(&percpu_threadpools, tpp, tpp_link);
}

static void
threadpool_remove_percpu(tpp)
	struct threadpool_percpu *tpp;
{
	KASSERT(threadpool_lookup_percpu(tpp->tpp_pri) == tpp);
	LIST_REMOVE(tpp, tpp_link);
}

void
threadpools_init(void)
{
	threadpool_thread_pc = (struct threadpool_thread *)malloc(sizeof(struct threadpool_thread), M_THREADPOOL, M_WAITOK);
	LIST_INIT(&unbound_threadpools);
	LIST_INIT(&percpu_threadpools);
	simple_lock_init(&threadpools_lock, "threadpools_lock");
}

/*
 * Thread pool creation
 */
static int
threadpool_create(pool, pri)
	struct threadpool *const pool;
	pri_t pri;
{
	struct proc *p;
	int ktflags, error;

	simple_lock_init(&pool->tp_lock, "pool_lock");
	/* XXX overseer */
	TAILQ_INIT(&pool->tp_jobs);
	TAILQ_INIT(&pool->tp_idle_threads);

	pool->tp_refcnt = 1;
	pool->tp_flags = 0;

	pool->tp_pri = pri;

	pool->tp_overseer.tpt_proc = NULL;
	pool->tp_overseer.tpt_pool = pool;
	pool->tp_overseer.tpt_job = NULL;

	ktflags = 0;
	if (pri) {
		error = kthread_create(&threadpool_overseer_thread, &pool->tp_overseer, &p,  "pooloverseer/%d@%d");
	}
	if (error) {
		goto fail0;
	}
	simple_lock(&pool->tp_lock);
	pool->tp_overseer.tpt_proc = p;
	wakeup(&pool->tp_overseer);
	simple_unlock(&pool->tp_lock);

	return (0);
fail0:
	KASSERT(error);
	KASSERT(pool->tp_overseer.tpt_job == NULL);
	KASSERT(pool->tp_overseer.tpt_pool == pool);
	KASSERT(pool->tp_flags == 0);
	KASSERT(pool->tp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&pool->tp_idle_threads));
	KASSERT(TAILQ_EMPTY(&pool->tp_jobs));
	return (error);
}

static void
threadpool_destroy(pool)
	struct threadpool *pool;
{
	struct threadpool_thread *thread;

	simple_lock(&pool->tp_lock);
	KASSERT(TAILQ_EMPTY(&pool->tp_jobs));
	pool->tp_flags |= THREADPOOL_DYING;
	wakeup(&pool->tp_overseer);
	TAILQ_FOREACH(thread, &pool->tp_idle_threads, tpt_entry) {
		wakeup(&thread);
	}
	while (0 < pool->tp_refcnt) {
		threadpool_wait(&pool->tp_overseer.tpt_proc, &pool->tp_overseer, &pool->tp_pri);
	}
	KASSERT(pool->tp_overseer.tpt_job == NULL);
	KASSERT(pool->tp_overseer.tpt_pool == pool);
	KASSERT(pool->tp_flags == THREADPOOL_DYING);
	KASSERT(pool->tp_refcnt == 0);
	KASSERT(TAILQ_EMPTY(&pool->tp_idle_threads));
	KASSERT(TAILQ_EMPTY(&pool->tp_jobs));

	simple_unlock(&pool->tp_lock);
}

static void
threadpool_hold(struct threadpool *pool)
{

//	KASSERT(mutex_owned(&pool->tp_lock));
	pool->tp_refcnt++;
	KASSERT(pool->tp_refcnt != 0);
}

static void
threadpool_rele(struct threadpool *pool)
{

	//KASSERT(mutex_owned(&pool->tp_lock));
	KASSERT(0 < pool->tp_refcnt);
	if (--pool->tp_refcnt == 0) {
		wakeup(&pool->tp_overseer);
	}
}

int
threadpool_get(poolp, pri)
	struct threadpool **poolp;
	pri_t pri;
{
	struct threadpool_unbound *tpu, *tmp = NULL;
	int error;

	simple_lock(&threadpools_lock);
	tpu = threadpool_lookup_unbound(pri);
	if (tpu == NULL) {
		simple_unlock(&threadpools_lock);
		tmp = malloc(sizeof(*tmp), M_THREADPOOL, M_WAITOK);
		error = threadpool_create(&tmp->tpu_pool, pri);
		if (error) {
			free(tmp, sizeof(*tmp));
			return error;
		}
		simple_lock(&threadpools_lock);
		tpu = threadpool_lookup_unbound(pri);
		if (tpu == NULL) {
			tpu = tmp;
			tmp = NULL;
			threadpool_insert_unbound(tpu);
		}
	}
	KASSERT(tpu != NULL);
	tpu->tpu_refcnt++;
	KASSERT(tpu->tpu_refcnt != 0);
	simple_unlock(&threadpools_lock);

	if (tmp != NULL) {
		threadpool_destroy(&tmp->tpu_pool);
		free(tmp, sizeof(*tmp));
	}
	KASSERT(tpu != NULL);
	*poolp = &tpu->tpu_pool;
	return (0);
}

void
threadpool_put(struct threadpool *pool, pri_t pri)
{
	struct threadpool_unbound *tpu;

	tpu = threadpool_lookup_unbound(pri);

	simple_lock(&threadpools_lock);
	KASSERT(tpu == threadpool_lookup_unbound(pri));
	KASSERT(0 < tpu->tpu_refcnt);
	if (--tpu->tpu_refcnt == 0) {
		threadpool_remove_unbound(tpu);
	} else {
		tpu = NULL;
	}
	simple_unlock(&threadpools_lock);

	if (tpu) {
		threadpool_destroy(&tpu->tpu_pool);
		free(tpu, sizeof(*tpu));
	}
}

/*
 * Threadpool Per-CPU
 */
int
threadpool_percpu_get(pool_percpup, pri)
	struct threadpool_percpu **pool_percpup;
	pri_t pri;
{
	struct threadpool_percpu *pool_percpu, *tmp = NULL;
	int error;

	simple_lock(&threadpools_lock);
	pool_percpu = threadpool_lookup_percpu(pri);
	if (pool_percpu == NULL) {
		simple_unlock(&threadpools_lock);
		error = threadpool_percpu_create(&tmp, pri);
		if (error)
			return error;
		KASSERT(tmp != NULL);
		simple_lock(&threadpools_lock);
		pool_percpu = threadpool_lookup_percpu(pri);
		if (pool_percpu == NULL) {
			pool_percpu = tmp;
			tmp = NULL;
			threadpool_insert_percpu(pool_percpu);
		}
	}
	KASSERT(pool_percpu != NULL);
	pool_percpu->tpp_refcnt++;
	KASSERT(pool_percpu->tpp_refcnt != 0);
	simple_unlock(&threadpools_lock);

	if (tmp != NULL) {
		threadpool_percpu_destroy(tmp);
	}
	KASSERT(pool_percpu != NULL);
	*pool_percpup = pool_percpu;
	return 0;
}

void
threadpool_percpu_put(pool_percpu, pri)
	struct threadpool_percpu *pool_percpu;
	pri_t pri;
{
	simple_lock(&threadpools_lock);
	KASSERT(pool_percpu == threadpool_lookup_percpu(pri));
	KASSERT(0 < pool_percpu->tpp_refcnt);
	if (--pool_percpu->tpp_refcnt == 0) {
		threadpool_remove_percpu(pool_percpu);
	} else {
		pool_percpu = NULL;
	}
	simple_unlock(&threadpools_lock);

	if (pool_percpu) {
		threadpool_percpu_destroy(pool_percpu);
	}
}

int
threadpool_percpu_start(pool_percpu, pri, ncpus)
	struct threadpool_percpu *pool_percpu;
	pri_t pri;
	int ncpus;
{
	pool_percpu = (struct threadpool_percpu *) malloc(sizeof(struct threadpool_percpu *), M_THREADPOOL, M_WAITOK);
	pool_percpu->tpp_pri = pri;

	percpu_foreach(pool_percpu->tpp_percpu, sizeof(struct threadpool *), 1, ncpus);

	if (pool_percpu->tpp_percpu != NULL) {
		/* Success!  */
		threadpool_percpu_init(pri);
		return (0);
	}

	threadpool_percpu_fini();
	percpu_extent_free(pool_percpu->tpp_percpu,
			pool_percpu->tpp_percpu->pc_start, pool_percpu->tpp_percpu->pc_end);
	free(pool_percpu, M_THREADPOOL);
	return (ENOMEM);
}

static int
threadpool_percpu_create(pool_percpup, pri)
	struct threadpool_percpu **pool_percpup;
	pri_t pri;
{
	struct threadpool_percpu *pool_percpu;
	int error, cpu;

	for (cpu = 0; cpu < cpu_number(); cpu++) {
		error = threadpool_percpu_start(pool_percpu, pri, cpu);
	}

	*pool_percpup = (struct threadpool_percpu *)pool_percpu;
	return (error);
}

static void
threadpool_percpu_destroy(pool_percpu)
	struct threadpool_percpu *pool_percpu;
{
	percpu_free(pool_percpu->tpp_percpu, sizeof(struct threadpool *));
	free(pool_percpu, M_THREADPOOL);
}

static void
threadpool_percpu_init(pri)
	pri_t pri;
{
	struct threadpool **const poolp;
	int error;

	*poolp = (struct threadpool *) malloc(sizeof(struct threadpool *), M_THREADPOOL, M_WAITOK);
	error = threadpool_create(*poolp, pri);
	if (error) {
		KASSERT(error == ENOMEM);
		free(*poolp, M_THREADPOOL);
		*poolp = NULL;
	}
}

static void
threadpool_percpu_fini(void)
{
	struct threadpool **const poolp;

	if (*poolp == NULL) {	/* initialization failed */
		return;
	}
	threadpool_destroy(*poolp);
	free(*poolp, M_THREADPOOL);
}

/*
 * Threadpool Jobs
 */
void
threadpool_job_init(job, func, lock, name)
	struct threadpool_job 	*job;
	threadpool_job_fn_t 	func;
	struct lock				*lock;
	char 					*name;
{
	job->job_lock = lock;
	job->job_name = name;
	job->job_refcnt = 0;
	job->job_thread = NULL;
	job->job_func = func;
}

void
threadpool_job_enqueue(pool, job, flag)
	struct threadpool 		*pool;
	struct threadpool_job 	*job;
	int 					flag;
{
	//threadpool_job_lock(job->job_lock);
	switch (flag) {
	case TPJ_HEAD:
		TAILQ_INSERT_HEAD(&pool->tp_jobs, job, job_entry);
		break;
	case TPJ_TAIL:
		 TAILQ_INSERT_TAIL(&pool->tp_jobs, job, job_entry);
		 break;
	}
	//threadpool_job_unlock(job->job_lock);
}

void
threadpool_job_dequeue(pool, job)
    struct threadpool       *pool;
    struct threadpool_job   *job;
{
    //threadpool_job_lock(job->job_lock);
    TAILQ_REMOVE(&pool->tp_jobs, job, job_entry);
    //threadpool_job_unlock(job->job_lock);
}

struct threadpool_job *
threadpool_job_search(pool, thread)
	struct threadpool 			*pool;
	struct threadpool_thread 	*thread;
{
	struct threadpool_job   *job;

	KASSERT(thread != NULL);
	threadpool_job_lock(job->job_lock);
	TAILQ_FOREACH(job, &pool->tp_jobs, job_entry) {
		if(job->job_thread == thread) {
			threadpool_job_unlock(job->job_lock);
			return (job);
		}
	}
	threadpool_job_unlock(job->job_lock);
	return (NULL);
}

void
threadpool_job_dead(job)
	struct threadpool_job *job;
{
	panic("threadpool job %p ran after destruction", job);
}

void
threadpool_job_destroy(job, pri)
	struct threadpool_job 	*job;
	int pri;
{
	KASSERTMSG((job->job_thread == NULL), "job %p still running", job);
	threadpool_job_lock(job->job_lock);
	while (0 < atomic_load_relaxed(&job->job_refcnt)) {
		threadpool_wait(&job->job_thread->tpt_proc, &job->job_thread, pri);
	}
	threadpool_job_unlock(job->job_lock);
	job->job_lock = NULL;
	KASSERT(job->job_thread == NULL);
	KASSERT(job->job_refcnt == 0);
	job->job_func = threadpool_job_dead;
	(void) strlcpy(job->job_name, "deadjob", sizeof(job->job_name));
}

void
threadpool_job_hold(job)
	struct threadpool_job *job;
{
	unsigned int refcnt;

	refcnt = atomic_inc_uint_nv(&job->job_refcnt);
	KASSERT(refcnt != 0);
}

void
threadpool_job_rele(job)
	struct threadpool_job *job;
{
	unsigned int refcnt;

	refcnt = atomic_dec_uint_nv(&job->job_refcnt);
	KASSERT(refcnt != UINT_MAX);
	if (refcnt == 0) {
		wakeup(&job->job_thread);
	}
}

void
threadpool_job_done(job)
	struct threadpool_job *job;
{
	KASSERT(job->job_thread != NULL);
	KASSERT(job->job_thread->tpt_proc == curproc);

	PROC_LOCK(curproc);
	curproc->p_name = job->job_thread->tpt_proc_savedname;
	PROC_UNLOCK(curproc);

	/*
	 * Inline the work of threadpool_job_rele(); the job is already
	 * locked, the most likely scenario (XXXJRT only scenario?) is
	 * that we're dropping the last reference (the one taken in
	 * threadpool_schedule_job()), and we always do the cv_broadcast()
	 * anyway.
	 */
	KASSERT(0 < atomic_load_relaxed(&job->job_refcnt));
	unsigned int refcnt __diagused = atomic_dec_int_nv(&job->job_refcnt);
	KASSERT(refcnt != UINT_MAX);
	wakeup(&job->job_thread);
	job->job_thread = NULL;
}

void
threadpool_schedule_job(pool, job, flags)
	struct threadpool 		*pool;
	struct threadpool_job 	*job;
	int 					flags;
{
	/*
	 * If the job's already running, let it keep running.  The job
	 * is guaranteed by the interlock not to end early -- if it had
	 * ended early, threadpool_job_done would have set job_thread
	 * to NULL under the interlock.
	 */
	if (__predict_true(job->job_thread != NULL)) {
		return;
	}

	threadpool_job_hold(job);

	/* Otherwise, try to assign a thread to the job.  */
	simple_lock(&pool->tp_lock);
	if (__predict_false(TAILQ_EMPTY(&pool->tp_idle_threads))) {
		/* Nobody's idle.  Give it to the dispatcher.  */
		job->job_thread = &pool->tp_overseer;
		threadpool_job_enqueue(pool, job, flags);
	} else {
		/* Assign it to the first idle thread.  */
		job->job_thread = TAILQ_FIRST(&pool->tp_idle_threads);
		threadpool_job_dequeue(pool, job);
		job->job_thread->tpt_job = job;
	}
	/* Notify whomever we gave it to, dispatcher or idle thread.  */
	KASSERT(job->job_thread != NULL);
	wakeup(&job->job_thread);
	simple_unlock(&pool->tp_lock);
}

bool_t
threadpool_cancel_job_async(pool, job)
	struct threadpool *pool;
	struct threadpool_job *job;
{
	if (job->job_thread == NULL) {
		/* Nothing to do.  Guaranteed not running.  */
		return (TRUE);
	} else if (job->job_thread == &pool->tp_overseer) {
		/* Take it off the list to guarantee it won't run.  */
		job->job_thread = NULL;
		simple_lock(&pool->tp_lock);
		threadpool_job_dequeue(pool, job);
		simple_unlock(&pool->tp_lock);
		threadpool_job_rele(job);
		return (TRUE);
	} else {
		return (FALSE);
	}
}

void
threadpool_cancel_job(pool, job)
	struct threadpool *pool;
	struct threadpool_job *job;
{
	if (threadpool_cancel_job_async(pool, job)) {
		return;
	}

	/* Already running.  Wait for it to complete.  */
	while (job->job_thread != NULL) {
		threadpool_wait(&job->job_thread->tpt_proc, &job->job_thread, &pool->tp_pri);
	}
}

static void threadpool_overseer_proc(struct proc *, struct threadpool_thread *const, struct threadpool *const);
static void threadpool_proc(struct proc *, struct threadpool_thread *const, struct threadpool *const);

static void
threadpool_overseer_thread(arg)
	void *arg;
{
	struct threadpool_thread *const overseer;

	overseer = arg;
	threadpool_overseer_proc(NULL, overseer, overseer->tpt_pool);
}

static void
threadpool_thread(arg)
	void *arg;
{
	struct threadpool_thread *const thread;

	thread = arg;
	threadpool_proc(curproc, thread, thread->tpt_pool);
}

static void
threadpool_wait(p, chan, pri)
	struct proc *p;
	const void *chan;
	int pri;
{
	sleep(chan, pri);
	unsleep(p);
}

static void
threadpool_overseer_proc(p, overseer, pool)
	struct proc *p;
	struct threadpool_thread *const overseer;
	struct threadpool *const pool;
{
	int ktflags;
	int error;

	p = NULL;
	/* Wait until we're initialized.  */
	simple_lock(&pool->tp_lock);
	while (overseer->tpt_proc == NULL) {
		threadpool_wait(overseer->tpt_proc, overseer, &pool->tp_pri);
	}

	for (;;) {
		/* Wait until there's a job.  */
		while (TAILQ_EMPTY(&pool->tp_jobs)) {
			if (ISSET(pool->tp_flags, THREADPOOL_DYING)) {
				break;
			}
			threadpool_wait(overseer->tpt_proc, overseer, &pool->tp_pri);
		}
		if (__predict_false(TAILQ_EMPTY(&pool->tp_jobs))) {
			break;
		}

		/* If there are no threads, we'll have to try to start one.  */
		if (TAILQ_EMPTY(&pool->tp_idle_threads)) {
			threadpool_hold(pool);
			simple_unlock(&pool->tp_lock);

			struct threadpool_thread *const thread = &threadpool_thread_pc;
			thread->tpt_proc = NULL;
			thread->tpt_pool = pool;
			thread->tpt_job = NULL;

			ktflags = 0;
			ktflags |= KTHREAD_MPSAFE;
			if (pool->tp_pri < PUSER) {
				ktflags |= KTHREAD_TS;
			}
			error = kthread_create(&threadpool_thread, thread, &p, "poolthread/%d@%d");

			simple_lock(&pool->tp_lock);
			if (error) {
				threadpool_rele(pool);
				continue;
			}
			/*
			 * New kthread now owns the reference to the pool
			 * taken above.
			 */
			KASSERT(p != NULL);
			TAILQ_INSERT_TAIL(&pool->tp_idle_threads, thread, tpt_entry);
			thread->tpt_proc = p;
			p = NULL;
			wakeup(&thread);
			continue;
		}

		/* There are idle threads, so try giving one a job.  */
		struct threadpool_job *const job = TAILQ_FIRST(&pool->tp_jobs);

		TAILQ_REMOVE(&pool->tp_jobs, job, job_entry);

		/*
		 * Take an extra reference on the job temporarily so that
		 * it won't disappear on us while we have both locks dropped.
		 */
		threadpool_job_hold(job);
		simple_unlock(&pool->tp_lock);

		simple_lock(job->job_lock);
		/* If the job was cancelled, we'll no longer be its thread. */
		if (__predict_true(job->job_thread == overseer)) {
			simple_lock(&pool->tp_lock);
			TAILQ_REMOVE(&pool->tp_jobs, job, job_entry);
			if (__predict_false(TAILQ_EMPTY(&pool->tp_idle_threads))) {
				/*
				 * Someone else snagged the thread
				 * first.  We'll have to try again.
				 */

				TAILQ_INSERT_HEAD(&pool->tp_jobs, job, job_entry);
			} else {
				/*
				 * Assign the job to the thread and
				 * wake the thread so it starts work.
				 */
				struct threadpool_thread *const thread = TAILQ_FIRST(&pool->tp_idle_threads);

				KASSERT(thread->tpt_job == NULL);
				TAILQ_REMOVE(&pool->tp_idle_threads, thread, tpt_entry);
				thread->tpt_job = job;
				job->job_thread = thread;
			}
			simple_unlock(&pool->tp_lock);
		}
		threadpool_job_rele(job);
		simple_unlock(job->job_lock);

		simple_lock(&pool->tp_lock);
	}
	threadpool_rele(pool);
	simple_unlock(&pool->tp_lock);

	kthread_exit(0);
}

static void
threadpool_proc(p, thread, pool)
	struct proc *p;
	struct threadpool_thread *const thread;
	struct threadpool *const pool;
{
	/* Wait until we're initialized and on the queue.  */
	simple_lock(&pool->tp_lock);

	KASSERT(thread->tpt_proc == p);
	for (;;) {
		/* Wait until we are assigned a job.  */
		while (thread->tpt_job == NULL) {
			if (ISSET(pool->tp_flags, THREADPOOL_DYING)) {
				break;
			}
		}
		if (__predict_false(thread->tpt_job == NULL)) {
			TAILQ_REMOVE(&pool->tp_idle_threads, thread, tpt_entry);
			break;
		}
		struct threadpool_job *const job = thread->tpt_job;
		KASSERT(job != NULL);

		/* Set our proc name to reflect what job we're doing.  */
		PROC_LOCK(p);
		char *const proc_name = p->p_name;
		thread->tpt_proc_savedname = p->p_name;
		p->p_name = job->job_name;
		PROC_UNLOCK(p);

		simple_unlock(&pool->tp_lock);

		/* Run the job.  */
		(*job->job_func)(job);

		/* proc name restored in threadpool_job_done(). */
		KASSERTMSG((p->p_name == proc_name), "someone forgot to call threadpool_job_done()!");

		/*
		 * We can compare pointers, but we can no longer deference
		 * job after this because threadpool_job_done() drops the
		 * last reference on the job while the job is locked.
		 */

		simple_lock(&pool->tp_lock);
		KASSERT(thread->tpt_job == job);
		thread->tpt_job = NULL;
		TAILQ_INSERT_TAIL(&pool->tp_idle_threads, thread, tpt_entry);
	}
	threadpool_rele(pool);
	simple_unlock(&pool->tp_lock);

	kthread_exit(0);
}
