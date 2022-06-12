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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/threadpool.h>
#include <sys/kthread.h>
#include <sys/percpu.h>

#include <vm/include/vm_param.h>

/* Threadpool Jobs */
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
	job->job_kthread = NULL;
	job->job_uthread = NULL;
	job->job_func = func;
}

void
threadpool_job_enqueue(jobpool, job, lock, flag)
	struct job_head 		*jobpool;
	struct threadpool_job 	*job;
	struct lock				*lock;
	int 					flag;
{
	threadpool_lock(lock);
	switch(flag) {
	case TPJ_HEAD:
		TAILQ_INSERT_HEAD(jobpool, job, job_entry);
		break;
	case TPJ_TAIL:
		 TAILQ_INSERT_TAIL(jobpool, job, job_entry);
		 break;
	}
    threadpool_unlock(lock);
}

void
threadpool_job_dequeue(jobpool, job, lock)
    struct job_head         *jobpool;
    struct threadpool_job   *job;
    struct lock			 	*lock;
{
    threadpool_lock(lock);
    TAILQ_REMOVE(jobpool, job, job_entry);
    threadpool_unlock(lock);
}

struct threadpool_job *
threadpool_job_search(jobpool, jobthread, thread, lock)
	struct job_head  *jobpool;
	void		     *jobthread;
	void 			 *thread;
	struct lock		 *lock;
{
	struct threadpool_job   *job;

	KASSERT(thread != NULL);
	threadpool_lock(lock);
	TAILQ_FOREACH(job, jobpool, job_entry) {
		if(jobthread == thread) {
			threadpool_unlock(lock);
			return (job);
		}
	}
	threadpool_unlock(lock);
	return (NULL);
}

void
threadpool_job_dead(job)
	struct threadpool_job *job;
{
	panic("threadpool job %p ran after destruction", job);
}

void
threadpool_job_destroy(thread, job)
	void 					*thread;
	struct threadpool_job 	*job;
{
	KASSERTMSG((thread == NULL), "job %p still running", job);
	job->job_lock = NULL;
	KASSERT(thread == NULL);
	KASSERT(job->job_refcnt == 0);
	job->job_func = threadpool_job_dead;
	(void) strlcpy(job->job_name, "deadjob", sizeof(job->job_name));
}

void
threadpool_job_hold(job)
	struct threadpool_job *job;
{
	unsigned int refcnt;

	do {
		refcnt = job->job_refcnt;
		KASSERT(refcnt != UINT_MAX);
	} while (atomic_cas_uint(&job->job_refcnt, refcnt, (refcnt + 1)) != refcnt);
}

void
threadpool_job_rele(job)
	struct threadpool_job *job;
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
threadpool_job_done(thread, p, job, name)
	void 				  *thread;
	struct proc 		  *p;
	struct threadpool_job *job;
	char 				  *name;
{
	KASSERT(thread != NULL);
	KASSERT(p == curproc());
	/*
	 * We can safely read this field; it's only modified right before
	 * we call the job work function, and we are only preserving it
	 * to use here; no one cares if it contains junk afterward.
	 */
	PROC_LOCK(curproc());
	curproc()->p_name = name;//job->job_kthread->ktpt_kthread_savedname;
	PROC_UNLOCK(curproc());

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
	thread = NULL;
}

void
threadpool_job_schedule(jobpool, job, lock, flag)
	struct job_head       *jobpool;
	struct threadpool_job *job;
	struct lock			  *lock;
	int 				  flag;
{
	threadpool_job_hold(job);
	threadpool_job_enqueue(jobpool, job, lock, flag);
}

void
threadpool_job_cancel(jobpool, job, lock)
 	struct job_head       *jobpool;
	struct threadpool_job *job;
	struct lock			  *lock;
{
	threadpool_job_dequeue(jobpool, job, lock);
	threadpool_job_rele(job);
}
