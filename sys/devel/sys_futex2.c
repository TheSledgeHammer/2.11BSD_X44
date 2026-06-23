/*	$NetBSD: sys_futex.c,v 1.26 2025/03/05 14:01:55 riastradh Exp $	*/

/*-
 * Copyright (c) 2018, 2019, 2020 The NetBSD Foundation, Inc.
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

/*
 * WIP
 *
 * TODO:
 * - further optimizations to the futex queue to improve insertion and removal.
 * - futex wake and requeue functions missing retval
 * - implement the futex syscall.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/fnv_hash.h>
#include <sys/lock.h>
#include <sys/queue.h>

#include <vm/include/vm.h>

#define M_FUTEX 105

#define futex_malloc(size, flags)	malloc(size, M_FUTEX, flags)
#define futex_free(addr)			free(addr, M_FUTEX)

union futex_key {
	struct vmspace *fk_vmspace;
	vm_object_t fk_object;
	vm_offset_t fk_offset;
};

struct futex_queue {
	struct futex *fq_futex;
	struct lock_object fq_lock;
	TAILQ_ENTRY(futex_queue) fq_node;
};

#define futex_lock_init(fq) 	simple_lock_init(&(fq)->fq_lock, "futex lock")
#define futex_lock(fq)			simple_lock(&(fq)->fq_lock)
#define futex_unlock(fq)		simple_unlock(&(fq)->fq_lock)

struct futex_head;
TAILQ_HEAD(futex_head, futex_queue);
struct futex {
	struct futex_head fx_head;		/* unused (see futex_hashtab) */
	struct lock_object fx_qlock;	/* futex qlock */
	union futex_key	fx_key;
	unsigned long fx_refcnt;		/* unused */
	struct proc *fx_procp; 			/* unused: futex proc */
	pid_t fx_pid;					/* unused: futex proc pid */
};

#define futex_qlock_init(fx) 	simple_lock_init(&(fx)->fx_qlock, "futex qlock")
#define futex_qlock(fx)			simple_lock(&(fx)->fx_qlock)
#define futex_qunlock(fx)		simple_unlock(&(fx)->fx_qlock)

#define FUTEX_SQSIZE 	128
#define FUTEX_HASH(x)	(((long)(x) >> 5) & (FUTEX_SQSIZE - 1))

static struct futex_head futex_hashtab[FUTEX_SQSIZE];

//static struct futex futex_tab;
//static struct futex_queue futex_slpque;

static void futex_key_init(union futex_key *, struct vmspace *, vm_offset_t);
static void futex_key_fini(union futex_key *);
static int futex_clock_gettime(clockid_t, struct timespec *);
static void futex_wait_abort(struct futex_queue *);
static int futex_wait(struct futex_queue *, const struct timespec *, clockid_t);
static int futex_wake(struct futex *, int, struct futex *, int);
static int futex_func_wait(struct vmspace *, vm_offset_t, const struct timespec *, clockid_t, int);
static int futex_func_wake(struct vmspace *, vm_offset_t, int);
static int futex_func_requeue(struct vmspace *, vm_offset_t, int, vm_offset_t, int);

static int
do_futex(struct vmspace *vmspace, int op, const struct timespec *timeout, clockid_t clock_id, vm_offset_t addr1, vm_offset_t addr2, int nwake, int nrequeue, int flags)
{
	int error;

	switch (op) {
	case FUTEX_WAIT:
		error = futex_func_wait(vmspace, addr1, timeout, clock_id, flags);
		break;
	case FUTEX_WAKE:
		error = futex_func_wake(vmspace, addr1, nwake);
		break;
	case FUTEX_REQUEUE:
		error = futex_func_requeue(vmspace, addr1, nwake, addr2, nrequeue);
		break;
	default:
		error = ENOSYS;
		break;
	}
	return (error);
}

int
futex()
{
	register struct futex_args {
		syscallarg(int *) addr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(const struct timespec *) timeout;
		syscallarg(int *) addr2;
		syscallarg(int) val2;
		syscallarg(int) val3;
	} uap = (struct futex_args *)u.u_ap;
	register struct proc *p;
	struct vmspace *vmspace;

	p = u.u_procp;
	vmspace = p->p_vmspace;

	do_futex(vmspace, SCARG(uap, op), SCARG(uap, timeout), SCARG(uap, addr), SCARG(uap, addr2), SCARG(uap, addr));
}

/* run in main.c */
void
futex_init(void)
{
	int i;

	for (i = 0; i < FUTEX_SQSIZE; i++) {
		TAILQ_INIT(&futex_hashtab[i]);
	}
    //futex_qlock_init(&futex_tab);
	//futex_lock_init(&futex_slpque);
}

static u_int32_t
futex_key_hash(union futex_key *key)
{
    u_int32_t hash1 = fnv_32_buf((vm_object_t)&key->fk_object, sizeof(&key->fk_object), FNV1_32_INIT);
    u_int32_t hash2 = fnv_32_buf(&key->fk_offset, sizeof(key->fk_offset), FNV1_32_INIT);
    hash1 ^= hash2;
    return (FUTEX_HASH(hash1));
}

static struct futex_head *
futex_hashtable(union futex_key *key)
{
	struct futex_head *head;

	head = &futex_hashtab[futex_key_hash(key)];
	if (head != NULL) {
		return (head);
	}
	return (NULL);
}

static struct futex *
futex_create(union futex_key *key)
{
	struct futex *fx;

	fx = (struct futex *)futex_malloc(*fx, M_WAITOK);
	if (fx == NULL) {
		futex_key_fini(key);
		return (NULL);
	}
	fx->fx_head = futex_hashtable(key);
    fx->fx_key = *key;
    fx->fx_refcnt = 1;
    futex_qlock_init(fx);
    return (fx);
}

static void
futex_key_init(union futex_key *fk, struct vmspace *vmspace, vm_offset_t addr)
{
	vm_map_t map;
	vm_map_entry_t entry;
	vm_object_t object;
	vm_offset_t eaddr, offset;
	vm_size_t elen;

	map = &vmspace->vm_map;
	vm_map_lock_read(map);
	if (vm_map_lookup_entry(map, addr, &entry)) {
		if (entry->inheritance == VM_INHERIT_SHARE) {
			elen = round_page((entry->end - entry->start));
			eaddr = ((entry->start + elen) - elen);
			if (eaddr == addr) {
				object = entry->object.vm_object;
				offset = entry->offset;
			}
		}
	}
	vm_map_unlock_read(map);
	fk->fk_vmspace = vmspace;
	fk->fk_object = object;
	fk->fk_offset = offset;
}

static void
futex_key_fini(union futex_key *fk)
{
	bzero(fk, sizeof(*fk));
}

static void
futex_queue_init(struct futex_queue *fq)
{
	struct futex_queue *result;

	result = (struct futex_queue *)futex_malloc(*result, M_WAITOK);
	if (result == NULL) {
		return;
	}
	futex_lock_init(result);
	result->fq_futex = NULL;
	fq = result;
}

static void
futex_queue_insert(struct futex_queue *fq, struct futex *fx, union futex_key *key)
{
	struct futex_head *head;

	head = futex_hashtable(key);
	futex_lock(fq);
	fq->fq_futex = fx;
	TAILQ_INSERT_TAIL(head, fq, fq_node);
	futex_unlock(fq);
}

static void
futex_queue_remove(struct futex_queue *fq, struct futex *fx, union futex_key *key)
{
	struct futex_head *head;

	head = futex_hashtable(key);
	futex_lock(fq);
	if ((fq->fq_futex == fx) && (&fx->fx_key == key)) {
		TAILQ_REMOVE(head, fq, fq_node);
		fq->fq_futex = NULL;
	}
	futex_unlock(fq);
}

struct futex_queue *
futex_queue_lookup(struct futex *fx, union futex_key *key)
{
	struct futex_head *head;
	struct futex_queue *fq;

	head = futex_hashtable(key);
	futex_lock(fq);
	TAILQ_FOREACH(fq, head, fq_node) {
		if ((fq->fq_futex == fx) && (&fx->fx_key == key)) {
			futex_unlock(fq);
			return (fq);
		}
	}
	futex_unlock(fq);
	return (NULL);
}

#ifdef notyet
void
futex_drain(struct futex *fx)
{
	struct futex_queue *fq;

	futex_qlock(fx);
	fq = futex_queue_lookup(fx, &fx->fx_key);
	if (fq != NULL) {
		futex_queue_remove(fq, fx, &fx->fx_key);
	}
	futex_qunlock(fx);
}

void
futex_destroy(struct futex *fx)
{
	futex_drain(fx);
	futex_free(fx);
}
#endif

int
futex_lookup_create(struct futex *fx0, struct vmspace *vmspace, vm_offset_t addr)
{
	union futex_key fk;
	struct futex *fx;
	struct futex_queue *fq;
	int error = 0;

	futex_key_init(&fk, vmspace, addr);

	fq = futex_queue_lookup(fx0, &fk);
	if (fq != NULL) {
		futex_key_fini(fk);
		goto out;
	} else {
		futex_queue_init(fq);
	}

	fx = futex_create(&fk);
	if (fx == NULL) {
		error = ENOMEM;
		goto out;
	}

	/*
	 * Insert our new futex, or use existing if someone else beat
	 * us to it.
	 */
	futex_queue_insert(fq, fx, &fk);
	if (fx == fx0) {
		fx = NULL;
	}

out:
	if (fx != NULL) {
		futex_free(fx);
	}
	return (error);
}

/* internal version of clock gettime */
static int
futex_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	struct timeval atv;
	struct timespec ats;
	int error, s;

	switch (clock_id) {
	case CLOCK_REALTIME:
		microtime(&atv);
		TIMEVAL_TO_TIMESPEC(&atv, &ats);
		break;
	case CLOCK_MONOTONIC:
		s = splclock();
		atv = mono_time;
		splx(s);
		TIMEVAL_TO_TIMESPEC(&atv, &ats);
		break;
	default:
		return (EINVAL);
	}
	bcopy(&ats, tp, sizeof(ats));
	return (0);
}

static void
futex_wait_abort(struct futex_queue *fq)
{
	struct futex *fx;

	fx = fq->fq_futex;

	futex_qlock(fx);
	futex_queue_remove(fq, fx, &fx->fx_key);
	futex_qunlock(fx);

	futex_lock(fq);
	KASSERT(fq->fq_futex == NULL);
}

static int
futex_wait(struct futex_queue *fq, const struct timespec *deadline, clockid_t clock_id)
{
	int error;

	error = 0;
	futex_lock(fq);
	for (;;) {
		if (fq->fq_futex == NULL) {
			error = 0;
			break;
		}

		if (error) {
			break;
		}

		if (deadline) {
			struct timespec ts;

			error = futex_clock_gettime(clock_id, &ts);
			if (error) {
				break;
			}

			if (timespeccmp(deadline, &ts, <=)) {
				error = ETIMEDOUT;
				break;
			}

			/* Count how much time is left.  */
			timespecsub(deadline, &ts, &ts);

			error = ltsleep(fq, PWAIT|PCATCH, "fsleep", MAX(1, tstohz(&ts)), &fq->fq_lock);
			if (error == EWOULDBLOCK) {
				error = 0;
			}
		} else {
			error = ltsleep(fq, PWAIT|PCATCH, "fsleep", 0, &fq->fq_lock);
		}
	}

	if (error) {
		futex_wait_abort(fq);
	}
	futex_unlock(fq);
	return (error);
}

static int
futex_wake(struct futex *fx1, int nwake, struct futex *fx2, int nrequeue)
{
	struct futex_head *head;
	struct futex_queue *fq;
	int nwoken_or_requeued = 0;

	head = futex_hashtable[futex_key_hash(&fx1->fx_key)];

	/* Wake up to nwake waiters, and count the number woken.  */
	TAILQ_FOREACH(fq, head, fq_node) {
		if (nwake > 0) {
			futex_queue_remove(fq, fx1, &fx1->fx_key);
			nwake--;
			nwoken_or_requeued++;
		} else {
			break;
		}
	}
	if (fx2) {
		/* Move up to nrequeue waiters from f's queue to f2's queue. */
		TAILQ_FOREACH(fq, head, fq_node) {
			if (nrequeue > 0) {
				futex_queue_remove(fq, fx1, &fx1->fx_key);
				futex_queue_insert(fq, fx2, &fx2->fx_key);
				nrequeue--;
				if (++nwoken_or_requeued == 0) { /* paranoia */
					nwoken_or_requeued = UINT_MAX;
				}
			} else {
				break;
			}
		}
	} else {
		KASSERT(nrequeue == 0);
	}
	/* Return the number of waiters woken or requeued. */
	return (nwoken_or_requeued);
}

static int
futex_func_wait(struct vmspace *vmspace, vm_offset_t addr, const struct timespec *timeout, clockid_t clock_id, int flags)
{
	struct futex_queue *fq;
	struct futex *fx;
	struct timespec ts;
	struct timeval tv;
	const struct timespec *deadline;
	int error;

	if (timeout == NULL || (flags & TIMER_ABSTIME) == TIMER_ABSTIME) {
		deadline = timeout;
	} else {
		error = futex_clock_gettime(clock_id, &ts);
		if (error) {
			return (error);
		}
		timespecadd(&ts, timeout, &ts);
		deadline = &ts;
	}

	error = futex_lookup_create(fx, vmspace, addr);
	if (error) {
		return (error);
	}

	KASSERT(fx);

	futex_queue_init(fq);

	futex_qlock(fx);
	futex_queue_insert(fq, fx, &fx->fx_key);
	futex_qunlock(fx);

	fx = NULL;

	error = futex_wait(fx, deadline, clock_id);
	if (error) {
		goto out;
	}

out:
	if (fx != NULL) {
		futex_free(fx);
	}
	return (error);
}

static int
futex_func_wake(struct vmspace *vmspace, vm_offset_t addr, int nwake)
{
	struct futex *fx;
	int error, nwoken;

	error = nwoken = 0;
	if (nwake < 0) {
		error = EINVAL;
		goto out;
	}

	error = futex_lookup_create(fx, vmspace, addr);
	if (error) {
		goto out;
	}
	if (fx == NULL) {
		goto out;
	}

	futex_qlock(fx);
	nwoken = futex_wake(fx, nwake, NULL, 0);
	futex_qunlock(fx);

out:
	return (error);
}

static int
futex_func_requeue(struct vmspace *vmspace, vm_offset_t addr, int nwake, vm_offset_t addr2, int nrequeue)
{
	struct futex *fx1, *fx2;
	int nwoken_or_requeued = 0;
	int error, cnt = 0;

	/* Reject negative number of wakeups or requeues. */
	if (nwake < 0 || nrequeue < 0) {
		error = EINVAL;
		goto out;
	}

	error = futex_lookup_create(fx1, vmspace, addr);
	if (error) {
		goto out;
	}

	if (fx1 == NULL) {
		goto out;
	}

	error = futex_lookup_create(fx2, vmspace, addr2);
	if (error) {
		goto out;
	}

	if ((fx1 != NULL) || (fx2 != NULL)) {
		if (fx1 != NULL) {
			futex_qlock(fx1);
		}
		if (fx2 != NULL) {
			futex_qlock(fx2);
		}
		error = 0;
		nwoken_or_requeued = futex_wake(fx1, nwake, fx2, nrequeue);
		if (fx1 != NULL) {
			futex_qunlock(fx1);
		}
		if (fx2 != NULL) {
			futex_qunlock(fx2);
		}
	} else {
		error = EAGAIN;
	}

out:
	if (fx2 != NULL) {
		futex_free(fx2);
	}
	if (fx1 != NULL) {
		futex_free(fx1);
	}
	return (error);
}
