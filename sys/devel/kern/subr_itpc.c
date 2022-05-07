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

#include <devel/sys/malloctypes.h>

/* Threadpool ITPC */
void
itpc_threadpool_init(void)
{
	itpc_setup(&itpc);
}

static void
itpc_setup(itpc)
	struct threadpool_itpc *itpc;
{
	MALLOC(itpc, struct threadpool_itpc *, sizeof(struct threadpool_itpc *), M_ITPC, NULL);
	TAILQ_INIT(itpc->itpc_header);
	itpc->itpc_refcnt = 0;
	itpc->itpc_jobs = NULL;
	itpc->itpc_job_name = NULL;
}

void
itpc_job_init(itpc, job, fmt, ap)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
	const char *fmt;
	va_list ap;
{
	if (job == itpc->itpc_jobs) {
		job->job_itpc = itpc;
		threadpool_job_init(job, job->job_func, job->job_lock, job->job_name, fmt, ap);
	}
}

void
itpc_job_dead(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_dead(job);
	}
}

void
itpc_job_destroy(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_destroy(job);
	}
}

void
itpc_job_hold(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_hold(job);
	}
}

void
itpc_job_rele(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_rele(job);
	}
}

void
itpc_job_done(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_done(job);
	}
}

struct kthreadpool
itpc_kthreadpool(itpc, ktpool)
	struct threadpool_itpc 	*itpc;
	struct kthreadpool 		*ktpool;
{
	if (itpc->itpc_ktpool == ktpool) {
		return (itpc->itpc_ktpool);
	}
	return (NULL);
}

struct uthreadpool
itpc_uthreadpool(itpc, utpool)
	struct threadpool_itpc 	*itpc;
	struct uthreadpool 		*utpool;
{
	if (itpc->itpc_utpool == utpool) {
		return (itpc->itpc_utpool);
	}
	return (NULL);
}

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
	job->job_itpc = NULL;
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
