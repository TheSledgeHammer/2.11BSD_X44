/*	$NetBSD: pthread_atfork.c,v 1.18 2024/01/20 14:52:47 christos Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: pthread_atfork.c,v 1.18 2024/01/20 14:52:47 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/queue.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "pthread.h"
#include "pthread_int.h"

__weak_alias(pthread_fork, _fork)

pid_t pthread_fork(void);
pid_t _fork(void);	/* XXX */

struct atfork_callback {
	SIMPLEQ_ENTRY(atfork_callback) next;
	void (*fn)(void);
};

/*
 * Hypothetically, we could protect the queues with a rwlock which is
 * write-locked by pthread_atfork() and read-locked by fork(), but
 * since the intended use of the functions is obtaining locks to hold
 * across the fork, forking is going to be serialized anyway.
 */
static pthread_mutex_t atfork_lock = PTHREAD_MUTEX_INITIALIZER;
SIMPLEQ_HEAD(atfork_callback_q, atfork_callback);

static struct atfork_callback_q prepareq = SIMPLEQ_HEAD_INITIALIZER(prepareq);
static struct atfork_callback_q parentq = SIMPLEQ_HEAD_INITIALIZER(parentq);
static struct atfork_callback_q childq = SIMPLEQ_HEAD_INITIALIZER(childq);

int
pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
	struct atfork_callback *newprepare, *newparent, *newchild;

	newprepare = newparent = newchild = NULL;

	if (prepare != NULL) {
		newprepare = malloc(sizeof(struct atfork_callback));
		if (newprepare == NULL) {
			return ENOMEM;
		}
		newprepare->fn = prepare;
	}

	if (parent != NULL) {
		newparent = malloc(sizeof(struct atfork_callback));
		if (newparent == NULL) {
			if (newprepare != NULL) {
				free(newprepare);
			}
			return (ENOMEM);
		}
		newparent->fn = parent;
	}

	if (child != NULL) {
		newchild = malloc(sizeof(struct atfork_callback));
		if (newchild == NULL) {
			if (newprepare != NULL) {
				free(newprepare);
			}
			if (newparent != NULL) {
				free(newparent);
			}
			return (ENOMEM);
		}
		newchild->fn = child;
	}

	pthread_mutex_lock(&atfork_lock);
	/*
	 * The order in which the functions are called is specified as
	 * LIFO for the prepare handler and FIFO for the others; insert
	 * at the head and tail as appropriate so that SIMPLEQ_FOREACH()
	 * produces the right order.
	 */
	if (prepare) {
		SIMPLEQ_INSERT_HEAD(&prepareq, newprepare, next);
	}
	if (parent) {
		SIMPLEQ_INSERT_TAIL(&parentq, newparent, next);
	}
	if (child) {
		SIMPLEQ_INSERT_TAIL(&childq, newchild, next);
	}
	pthread_mutex_unlock(&atfork_lock);
	return (0);
}

pid_t
pthread_fork(void)
{
	struct atfork_callback *iter;
	pid_t ret;

	pthread_mutex_lock(&atfork_lock);
	SIMPLEQ_FOREACH(iter, &prepareq, next) {
		(*iter->fn)();
	}

	ret = _fork();

	if (ret != 0) {
		/*
		 * We are the parent. It doesn't matter here whether
		 * the fork call succeeded or failed.
		 */
		SIMPLEQ_FOREACH(iter, &parentq, next) {
			(*iter->fn)();
		}
		pthread_mutex_unlock(&atfork_lock);
	} else {
		/* We are the child */
		SIMPLEQ_FOREACH(iter, &childq, next) {
			(*iter->fn)();
		}
		/*
		 * Note: We are explicitly *not* unlocking
		 * atfork_lock.  Unlocking atfork_lock is problematic,
		 * because if any threads in the parent blocked on it
		 * between the initial lock and the fork() syscall,
		 * unlocking in the child will try to schedule
		 * threads, and either the internal mutex interlock or
		 * the runqueue spinlock could have been held at the
		 * moment of fork(). Since the other threads do not
		 * exist in this process, the spinlock will never be
		 * unlocked, and we would wedge.
		 * Instead, we reinitialize atfork_lock, since we know
		 * that the state of the atfork lists is consistent here,
		 * and that there are no other threads to be affected by
		 * the forcible cleaning of the queue.
		 * This permits double-forking to work, although
		 * it requires knowing that it's "safe" to initialize
		 * a locked mutex in this context.
		 *
		 * The problem exists for users of this interface,
		 * too, since the intented use of pthread_atfork() is
		 * to acquire locks across the fork call to ensure
		 * that the child sees consistent state. There's not
		 * much that can usefully be done in a child handler,
		 * and conventional wisdom discourages using them, but
		 * they're part of the interface, so here we are...
		 */
		pthread_mutex_init(&atfork_lock, NULL);
	}

	return (ret);
}
