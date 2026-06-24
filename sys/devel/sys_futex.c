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
 * TODO:
 * - missing wakeup in futex_queue_abort?
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/fnv_hash.h>
#include <sys/syscall.h>
#include <sys/sysdecl.h>

#include <devel/futex.h>

#include <vm/include/vm.h>

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

#define futex_lock(fq)			simple_lock(&(fq)->fq_lock)
#define futex_unlock(fq)		simple_unlock(&(fq)->fq_lock)

struct futex_head;
TAILQ_HEAD(futex_head, futex_queue);
struct futex {
	struct futex_head fx_head;		/* unused (see futex_hashtab) */
	struct lock_object fx_qlock;	/* futex qlock */
	union futex_key	fx_key;
	unsigned long fx_refcnt;		/* unused */
	//struct proc *fx_procp; 			/* unused: futex proc */
	//pid_t fx_pid;					/* unused: futex proc pid */
};

#define futex_qlock(fx)			simple_lock(&(fx)->fx_qlock)
#define futex_qunlock(fx)		simple_unlock(&(fx)->fx_qlock)

#define FUTEX_SQSIZE 	128
#define FUTEX_HASH(x)	(((long)(x) >> 5) & (FUTEX_SQSIZE - 1))

static struct lock_object futex_lock;
static struct lock_object futex_qlock;
static struct futex_head futex_hashtab[FUTEX_SQSIZE];

static int futex_load(vm_offset_t *, vm_offset_t *);
static bool_t futex_test(vm_offset_t *, vm_offset_t);
static u_int32_t futex_key_hash(union futex_key *);
static struct futex_head *futex_hashtable(union futex_key *);
static struct futex *futex_create(union futex_key *);
static void futex_key_init(union futex_key *, struct vmspace *, vm_offset_t *);
static void futex_key_fini(union futex_key *);
static void futex_queue_init(struct futex_queue *);
static void futex_queue_insert(struct futex_queue *, struct futex *, union futex_key *);
static struct futex_queue *futex_queue_lookup(struct futex *, union futex_key *);
static int futex_lookup_create(struct futex *, struct vmspace *, vm_offset_t *);
static int futex_clock_gettime(clockid_t, struct timespec *);
static void futex_wait_abort(struct futex_queue *);
static int futex_wait(struct futex_queue *, const struct timespec *, clockid_t);
static int futex_wake(struct futex *, int, struct futex *, int);
static int futex_func_wait(struct vmspace *, vm_offset_t *, vm_offset_t, const struct timespec *, clockid_t, int, register_t *);
static int futex_func_wake(struct vmspace *, vm_offset_t, int, register_t *);
static int futex_func_requeue(struct vmspace *, vm_offset_t *, vm_offset_t, int, vm_offset_t *, int, register_t *);

static int
do_futex(p, op, timeout, clock_id, addr1, val, nwake, addr2, nrequeue, flags)
	struct proc *p;
	int op;
	const struct timespec *timeout;
	clockid_t clock_id;
	vm_offset_t *addr1;
	vm_offset_t val;
	vm_offset_t *addr2;
	int nwake;
	int nrequeue;
	int flags;
{
	struct vmspace *vmspace;
	int error;
	register_t retval;

	vmspace = p->p_vmspace;
	//flags = op & FUTEX_FLAG_MASK;
	op &= FUTEX_OP_MASK;
	switch (op) {
	case FUTEX_WAIT:
		u.u_error = futex_func_wait(vmspace, addr1, val, timeout, clock_id, flags, &retval);
		u.u_r.r_val1 = retval;
		break;
	case FUTEX_WAKE:
		u.u_error = futex_func_wake(vmspace, addr1, nwake, &retval);
		u.u_r.r_val1 = retval;
		break;
	case FUTEX_REQUEUE:
		u.u_error = futex_func_requeue(vmspace, addr1, val, nwake, addr2, nrequeue, &retval);
		u.u_r.r_val1 = retval;
		break;
	default:
		u.u_error = ENOSYS;
		break;
	}
	return (u.u_error);
}

int
futex()
{
	register struct futex_args {
		syscallarg(int) op;
		syscallarg(const struct timespec *) timeout;
		syscallarg(clockid_t) clock_id;
		syscallarg(u_long *) addr1;
		syscallarg(u_long) val;
		syscallarg(int) nwake;
		syscallarg(u_long *) addr2;
		syscallarg(int) nrequeue;
		syscallarg(int) flags;
	} uap = (struct futex_args *)u.u_ap;
	register struct proc *p;
	struct timespec ts, *tsp;

	p = u.u_procp;
	if (SCARG(uap, timeout)) {
		u.u_error = copyin(SCARG(uap, timeout), &ts, sizeof(ts));
		if (u.u_error) {
			return (u.u_error);
		}
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	u.u_error = do_futex(p, SCARG(uap, op), tsp, SCARG(uap, clock_id),
			SCARG(uap, addr1), SCARG(uap, val), SCARG(uap, nwake),
			SCARG(uap, addr2), SCARG(uap, nrequeue), SCARG(uap, flags));
	return (u.u_error);
}

static int
futex_load(uaddr, kaddr)
	vm_offset_t *uaddr;
	vm_offset_t *kaddr;
{
	return (copyin((vm_offset_t *)uaddr, (vm_offset_t *)kaddr, sizeof(*kaddr)));
}

/*
 * futex_test(uaddr, expected)
 *
 *	True if *uaddr == expected.  False if *uaddr != expected, or if
 *	uaddr is not mapped.
 */
static bool_t
futex_test(uaddr, expected)
	vm_offset_t *uaddr;
	vm_offset_t expected;
{
	vm_offset_t val;
	int error;

	error = futex_load(uaddr, &val);
	if (error) {
		return (FALSE);
	}
	return (val == expected);
}

/* run in main.c */
void
futex_init(void)
{
	int i;

	for (i = 0; i < FUTEX_SQSIZE; i++) {
		TAILQ_INIT(&futex_hashtab[i]);
	}
	simple_lock_init(&futex_lock, "futex lock");
	simple_lock_init(&futex_qlock, "futex qlock");
}

static u_int32_t
futex_key_hash(key)
	union futex_key *key;
{
    u_int32_t hash1 = fnv_32_buf((vm_object_t)&key->fk_object, sizeof(&key->fk_object), FNV1_32_INIT);
    u_int32_t hash2 = fnv_32_buf(&key->fk_offset, sizeof(key->fk_offset), FNV1_32_INIT);
    hash1 ^= hash2;
    return (FUTEX_HASH(hash1));
}

static struct futex_head *
futex_hashtable(key)
	union futex_key *key;
{
	struct futex_head *head;

	head = &futex_hashtab[futex_key_hash(key)];
	if (head != NULL) {
		return (head);
	}
	return (NULL);
}

static struct futex *
futex_create(key)
	union futex_key *key;
{
	struct futex *fx;

	fx = (struct futex *)futex_malloc(sizeof(*fx), M_WAITOK);
	if (fx == NULL) {
		futex_key_fini(key);
		return (NULL);
	}
	fx->fx_head = futex_hashtable(key);
	fx->fx_key = *key;
	fx->fx_refcnt = 1;
	fx->fx_qlock = futex_qlock;
	return (fx);
}

static void
futex_key_init(key, vmspace, uaddr)
	union futex_key *key;
	struct vmspace *vmspace;
	vm_offset_t *uaddr;
{
	vm_map_t map;
	vm_map_entry_t entry;
	vm_object_t object;
	//vm_amap_t amap;
	vm_offset_t eaddr, offset, addr;
	vm_size_t elen;

	addr = *uaddr;
	map = &vmspace->vm_map;
	vm_map_lock_read(map);
	if (vm_map_lookup_entry(map, addr, &entry)) {
		if (entry->inheritance == VM_INHERIT_SHARE) {
			elen = round_page((entry->end - entry->start));
			eaddr = ((entry->start + elen) - elen);
			if (eaddr == addr) {
				object = entry->object.vm_object;
				//amap = entry->aref.ar_amap;
				offset = entry->offset;
			}
		}
	}
	vm_map_unlock_read(map);
	key->fk_vmspace = vmspace;
	key->fk_object = object;
	//key->fk_amap = amap;
	key->fk_offset = offset;
}

static void
futex_key_fini(key)
	union futex_key *key;
{
	bzero(key, sizeof(*key));
}

static void
futex_queue_init(fq)
	struct futex_queue *fq;
{
	struct futex_queue *result;

	if (fq != NULL) {
		fq->fq_lock = futex_lock;
		fq->fq_futex = NULL;
	} else {
		result = (struct futex_queue *)futex_malloc(sizeof(*result), M_WAITOK);
		if (result == NULL) {
			return;
		}
		result->fq_lock = futex_lock;
		result->fq_futex = NULL;
		fq = result;
	}
}

static void
futex_queue_insert(fq, fx, key)
	struct futex_queue *fq;
	struct futex *fx;
	union futex_key *key;
{
	struct futex_head *head;

	head = futex_hashtable(key);
	futex_lock(fq);
	fq->fq_futex = fx;
	TAILQ_INSERT_TAIL(head, fq, fq_node);
	futex_unlock(fq);
}

static void
futex_queue_remove(fq, fx, key)
	struct futex_queue *fq;
	struct futex *fx;
	union futex_key *key;
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

static struct futex_queue *
futex_queue_lookup(fx, key)
	struct futex *fx;
	union futex_key *key;
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

static int
futex_lookup_create(fx0, vmspace, addr)
	struct futex *fx0;
	struct vmspace *vmspace;
	vm_offset_t *addr;
{
	union futex_key fk;
	struct futex *fx;
	struct futex_queue *fq;
	int error = 0;

	futex_key_init(&fk, vmspace, addr);

	fq = futex_queue_lookup(fx0, &fk);
	if (fq != NULL) {
		/*
		 * We either found one, or there was an error.
		 * In either case, we are done with the key.
		 */
		futex_key_fini(fk);
		goto out;
	} else {
		futex_queue_init(fq);
	}

	/*
	 * Create a futex record.  This transfers ownership of the key
	 * in all cases.
	 */
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

	/* Success!  */
	KASSERT(error == 0);
out:
	if (fx != NULL) {
		futex_free(fx);
	}
	KASSERT(error || fx0 != NULL);
	return (error);
}

/* internal version of clock gettime */
static int
futex_clock_gettime(clock_id, tp)
	clockid_t clock_id;
	struct timespec *tp;
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

/*
 * futex_wait_abort(fq)
 *
 *	Caller is no longer waiting for fq.  Remove it from any queue
 *	if it was on one.  Caller must hold fq->fq_lock.
 */
static void
futex_wait_abort(fq)
	struct futex_queue *fq;
{
	struct futex *fx;

	/*
	 * Grab the futex queue.  It can't go away as long as we hold
	 * fq_lock.  However, we can't take the queue lock because
	 * that's a lock order reversal.
	 */
	fx = fq->fq_futex;

	futex_qlock(fx);
	futex_queue_remove(fq, fx, &fx->fx_key);
	futex_qunlock(fx);

	wakeup((struct futex *)fx);

	futex_lock(fq);
	KASSERT(fq->fq_futex == NULL);
}

static int
futex_wait(fq, deadline, clock_id)
	struct futex_queue *fq;
	const struct timespec *deadline;
	clockid_t clock_id;
{
	int error;

	error = 0;
	/* Test and wait under the wait lock.  */
	futex_lock(fq);
	for (;;) {
		/* If we're done yet, stop and report success.  */
		if (fq->fq_futex == NULL) {
			error = 0;
			break;
		}

		/* If anything went wrong in the last iteration, stop.  */
		if (error) {
			break;
		}

		/* Not done yet.  Wait.  */
		if (deadline) {
			struct timespec ts;

			/* Check our watch.  */
			error = futex_clock_gettime(clock_id, &ts);
			if (error) {
				break;
			}

			/* If we're past the deadline, ETIMEDOUT.  */
			if (timespeccmp(deadline, &ts, <=)) {
				error = ETIMEDOUT;
				break;
			}

			/* Count how much time is left.  */
			timespecsub(deadline, &ts, &ts);

			/*
			 * Wait for that much time, allowing signals.
			 * Ignore EWOULDBLOCK, however: we will detect
			 * timeout ourselves on the next iteration of
			 * the loop -- and the timeout may have been
			 * truncated by tstohz, anyway.
			 */
			error = ltsleep(fq, PWAIT|PCATCH, "fsleep", MAX(1, tstohz(&ts)), &fq->fq_lock);
			if (error == EWOULDBLOCK) {
				error = 0;
			}
		} else {
			/* Wait indefinitely, allowing signals. */
			error = ltsleep(fq, PWAIT|PCATCH, "fsleep", 0, &fq->fq_lock);
		}
	}

	/*
	 * If we were woken up, the waker will have removed fq from the
	 * queue.  But if anything went wrong, we must remove fq from
	 * the queue ourselves.
	 */
	if (error) {
		futex_wait_abort(fq);
	}
	futex_unlock(fq);
	return (error);
}

static int
futex_wake(fx1, nwake, fx2, nrequeue)
	struct futex *fx1;
	int nwake;
	struct futex *fx2;
	int nrequeue;
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
futex_func_wait(vmspace, addr, val, timeout, clock_id, flags, retval)
	struct vmspace *vmspace;
	vm_offset_t *addr;
	vm_offset_t val;
	const struct timespec *timeout;
	clockid_t clock_id;
	int flags;
	register_t *retval;
{
	struct futex_queue *fq;
	struct futex *fx;
	struct timespec ts;
	struct timeval tv;
	const struct timespec *deadline;
	int error;

	if (!futex_test(addr, val)) {
		return (EAGAIN);
	}

	/* Determine a deadline on the specified clock.  */
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

	/* Get the futex, creating it if necessary.  */
	error = futex_lookup_create(fx, vmspace, addr);
	if (error) {
		return (error);
	}

	KASSERT(fx);

	/* Get ready to wait.  */
	futex_queue_init(fq);

	futex_qlock(fx);
	if (!futex_test(addr, val)) {
		futex_qunlock(fx);
		error = EAGAIN;
		goto out;
	}

	futex_queue_insert(fq, fx, &fx->fx_key);
	futex_qunlock(fx);

	/*
	 * We cannot drop our reference to the futex here, because
	 * we might be enqueued on a different one when we are awakened.
	 * The references will be managed on our behalf in the requeue
	 * and wake cases.
	 */
	fx = NULL;

	/* Wait. */
	error = futex_wait(fx, deadline, clock_id);
	if (error) {
		goto out;
	}

	/* Return 0 on success, error on failure. */
	retval = 0;

out:
	if (fx != NULL) {
		futex_free(fx);
	}
	return (error);
}

static int
futex_func_wake(vmspace, addr, nwake, retval)
	struct vmspace *vmspace;
	vm_offset_t *addr;
	int nwake;
	register_t *retval;
{
	struct futex *fx;
	int error, nwoken;

	error = nwoken = 0;
	/* Reject negative number of wakeups.  */
	if (nwake < 0) {
		error = EINVAL;
		goto out;
	}

	/* Look up the futex, if any.  */
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
	/* Return the number of waiters woken.  */
	retval = nwoken;

	/* Success!  */
	return (error);
}

static int
futex_func_requeue(vmspace, addr, val, nwake, addr2, nrequeue, retval)
	struct vmspace *vmspace;
	vm_offset_t *addr;
	vm_offset_t val;
	int nwake;
	vm_offset_t *addr2;
	int nrequeue;
	register_t *retval;
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

	/* If there is none for FUTEX_REQUEUE, nothing to do. */
	if (fx1 == NULL) {
		goto out;
	}

	/*
	 * We may need to create the destination futex because it's
	 * entirely possible it does not currently have any waiters.
	 */
	error = futex_lookup_create(fx2, vmspace, addr2);
	if (error) {
		goto out;
	}

	if (!futex_test(addr, val)) {
		error = EAGAIN;
	} else {
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
	}

out:
	/* Return the number of waiters woken or requeued.  */
	retval = nwoken_or_requeued;

	/* Release the futexes if we got them.  */
	if (fx2 != NULL) {
		futex_free(fx2);
	}
	if (fx1 != NULL) {
		futex_free(fx1);
	}
	return (error);
}
