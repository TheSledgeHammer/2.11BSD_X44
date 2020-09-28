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

#include <sys/lock.h>
#include <sys/user.h>
#include <workqueue.h>

/* workqueue */
void
wqueue_alloc(wq)
	struct wqueue 	*wq;
{
	if(wq == NULL) {
		MALLOC(wq, struct wqueue *, sizeof(struct wqueue *), M_WQUEUE, NULL);
	}
	TAILQ_INIT(&wq->wq_head);
}

void
wqueue_free(wq)
	struct wqueue 	*wq;
{
	if(wq != NULL) {
		FREE(wq, M_WQUEUE);
	}
}

struct wqueue *
wqueue_create(wq, name, nthreads, flags)
	struct wqueue 	*wq;
	const char		*name;
	int 			nthreads;
	int 			flags;
{
	wqueue_alloc(wq);

	if(wq == NULL) {
		return (NULL);
	}

	wq->wq_name = name;
	wq->wq_nthreads = nthreads;
	wq->wq_state = WQ_CREATED;
	wq->wq_flags = flags;

	return (wq);
}

void
wqueue_destroy(wq, name)
	struct wqueue 	*wq;
	const char		*name;
{
	if(wq->wq_name == name) {
		if(wq->wq_state == WQ_CREATED) {
			wq->wq_state = WQ_DESTROYED;
			wqueue_free(wq);
		}
		if (wq->wq_state == WQ_DESTROYED){
			wqueue_free(wq);
		}
	} else {
		panic("unexpected %s wq state %u", wq->wq_name, wq->wq_state);
	}
}

/* tasks */
void
task_set(tk, fn, arg)
	struct task *tk;
	void (*fn)(void *);
	void *arg;
{
	tk->tk_func = fn;
	tk->tk_arg = arg;
	tk->tk_flags = 0;
	tk->tk_state = 0;
}

struct task *
task_lookup(wq, tk)
    struct wqueue *wq;
    struct task *tk;
{
    struct task *entry;
    for (entry = TAILQ_FIRST(&wq->wq_head); entry != NULL; entry = TAILQ_NEXT(tk, tk_entry)) {
        if(tk == entry) {
            return (tk);
        }
    }
    return (NULL);
}

void
task_add(wq, tk)
	struct wqueue *wq;
	struct task *tk;
{
	TAILQ_INSERT_HEAD(&wq->wq_head, tk, tk_entry);
}

void
task_remove(wq, tk)
	struct wqueue *wq;
	struct task *tk;
{
	struct task *entry;
	entry = task_lookup(wq, tk);
	if (entry != NULL) {
		TAILQ_REMOVE(&wq->wq_head, entry, tk_entry);
	}
}

bool_t
task_check(wq, tk)
    struct wqueue *wq;
    struct task *tk;
{
    if(wq == NULL || tk == NULL) {
        return (FALSE);
    }
    if(task_lookup(wq, tk) != NULL) {
        return (TRUE);
    }
    return (FALSE);
}

/* Job Pool */
void
job_pool_task_run(job, wq, tk)
	struct threadpool_job 	*job;
	struct wqueue 			*wq;
	struct task 			*tk;
{
	struct task *entry;
	if (task_check(wq, tk)) {
		entry = task_lookup(wq, tk);
		if (entry) {
			/* run task */
			((*job->job_func)(job, wq, entry, entry->tk_func));
		}
	} else {
		panic("no wqueue tasks found in job pool");
	}
}

void
job_pool_task_enqueue(ktpool, job)
	struct kthreadpool 		*ktpool;
    struct threadpool_job	*job;
{
   TAILQ_INSERT_TAIL(&ktpool->ktp_jobs, job, job_entry);
}

void
job_pool_task_dequeue(ktpool, job)
	struct kthreadpool		*ktpool;
	struct threadpool_job	*job;
{
	TAILQ_REMOVE(&ktpool->ktp_jobs, job, job_entry);
}
