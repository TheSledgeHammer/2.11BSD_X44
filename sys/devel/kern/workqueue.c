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

struct wqueue *
wqueue_create(wq, nthreads)
	struct wqueue *wq;
	int nthreads;
{
	LIST_INIT(&wq->wq_pending);
	LIST_INIT(&wq->wq_running);
	wq->wq_nthreads = nthreads;

	return (wq);
}

void
task_set(tk, prio, wmesg, timo, flags, state)
	struct task *tk;
	int prio, timo, flags;
	char *wmesg;
	int state;
{
	tk->tk_prio = prio;
	tk->tk_wmesg = wmesg;
	tk->tk_timo = timo;
	tk->tk_flags = flags;
	tk->tk_state = state;
	lockinit(tk->tk_lock, prio, wmesg, flags);
}

struct task *
task_lookup(wq, tk, state)
	struct wqueue *wq;
	struct task *tk;
	int state;
{
	struct task *entry;
	if (tk->tk_state == state) {
		for (entry = LIST_FIRST(&wq->wq_pending); entry != NULL; entry = LIST_NEXT(tk, tk_entry)) {
			if(tk == entry) {
				return (tk);
			}
		}
	}
	return (NULL);
}

void
task_add(wq, tk)
	struct wqueue *wq;
	struct task *tk;
{
	if(tk->tk_state == TQ_PENDING) {
		LIST_INSERT_HEAD(&wq->wq_pending, tk, tk_entry);
	}
	if(tk->tk_state == TQ_RUNNING) {
		LIST_INSERT_HEAD(&wq->wq_running, tk, tk_entry);
	}
}

void
task_remove(wq, tk)
	struct wqueue *wq;
	struct task *tk;
{
	struct task *entry;
	if (tk->tk_state == TQ_PENDING) {
		entry = task_lookup(wq, tk, TQ_PENDING);
		if(entry != NULL) {
			LIST_REMOVE(entry, tk_entry);
		}
	}
	if (tk->tk_state == TQ_RUNNING) {
		entry = task_lookup(wq, tk, TQ_RUNNING);
		if(entry != NULL) {
			LIST_REMOVE(entry, tk_entry);
		}
	}
}
