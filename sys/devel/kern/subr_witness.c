/*	$OpenBSD: subr_witness.c,v 1.37 2020/03/15 05:58:48 visa Exp $	*/

/*-
 * Copyright (c) 2008 Isilon Systems, Inc.
 * Copyright (c) 2008 Ilya Maykov <ivmaykov@gmail.com>
 * Copyright (c) 1998 Berkeley Software Design, Inc.
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
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from BSDI Id: mutex_witness.c,v 1.1.2.20 2000/04/27 03:10:27 cp Exp
 *	and BSDI Id: synch_machdep.c,v 2.3.2.39 2000/04/27 03:10:25 cp Exp
 */

/*
 * Implementation of the `witness' lock verifier.  Originally implemented for
 * mutexes in BSD/OS.  Extended to handle generic lock objects and lock
 * classes in FreeBSD.
 */

/*
 *	Main Entry: witness
 *	Pronunciation: 'wit-n&s
 *	Function: noun
 *	Etymology: Middle English witnesse, from Old English witnes knowledge,
 *	    testimony, witness, from 2wit
 *	Date: before 12th century
 *	1 : attestation of a fact or event : TESTIMONY
 *	2 : one that gives evidence; specifically : one who testifies in
 *	    a cause or before a judicial tribunal
 *	3 : one asked to be present at a transaction so as to be able to
 *	    testify to its having taken place
 *	4 : one who has personal knowledge of something
 *	5 a : something serving as evidence or proof : SIGN
 *	  b : public affirmation by word or example of usually
 *	      religious faith or conviction <the heroic witness to divine
 *	      life -- Pilot>
 *	6 capitalized : a member of the Jehovah's Witnesses
 */

/*
 * Special rules concerning Giant and lock orders:
 *
 * 1) Giant must be acquired before any other mutexes.  Stated another way,
 *    no other mutex may be held when Giant is acquired.
 *
 * 2) Giant must be released when blocking on a sleepable lock.
 *
 * This rule is less obvious, but is a result of Giant providing the same
 * semantics as spl().  Basically, when a thread sleeps, it must release
 * Giant.  When a thread blocks on a sleepable lock, it sleeps.  Hence rule
 * 2).
 *
 * 3) Giant may be acquired before or after sleepable locks.
 *
 * This rule is also not quite as obvious.  Giant may be acquired after
 * a sleepable lock because it is a non-sleepable lock and non-sleepable
 * locks may always be acquired while holding a sleepable lock.  The second
 * case, Giant before a sleepable lock, follows from rule 2) above.  Suppose
 * you have two threads T1 and T2 and a sleepable lock X.  Suppose that T1
 * acquires X and blocks on Giant.  Then suppose that T2 acquires Giant and
 * blocks on X.  When T2 blocks on X, T2 will release Giant allowing T1 to
 * execute.  Thus, acquiring Giant both before and after a sleepable lock
 * will not result in a lock order reversal.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/lockobj.h>
#include <sys/witness.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
//#include <ddb/db_var.h>
#include <ddb/db_output.h>

#define	LI_RECURSEMASK			0x0000ffff	/* Recursion depth of lock instance. */
#define	LI_EXCLUSIVE			0x00010000	/* Exclusive lock instance. */
#define	LI_NORELEASE			0x00020000	/* Lock not allowed to be released. */

#ifndef WITNESS_COUNT
#define	WITNESS_COUNT			1536
#endif
#define	WITNESS_HASH_SIZE		251			/* Prime, gives load factor < 2 */
#define	WITNESS_PENDLIST		(1024 + MAXCPUS)

/* Allocate 256 KB of stack data space */
#define	WITNESS_LO_DATA_COUNT	2048

/* Prime, gives load factor of ~2 at full load */
#define	WITNESS_LO_HASH_SIZE	1021

/*
 * XXX: This is somewhat bogus, as we assume here that at most 2048 threads
 * will hold LOCK_NCHILDREN locks.  We handle failure ok, and we should
 * probably be safe for the most part, but it's still a SWAG.
 */
#define	LOCK_NCHILDREN			5
#define	LOCK_CHILDCOUNT			2048

#define	FULLGRAPH_SBUF_SIZE		512

/*
 * These flags go in the witness relationship matrix and describe the
 * relationship between any two struct witness objects.
 */
#define	WITNESS_UNRELATED        0x00    /* No lock order relation. */
#define	WITNESS_PARENT           0x01    /* Parent, aka direct ancestor. */
#define	WITNESS_ANCESTOR         0x02    /* Direct or indirect ancestor. */
#define	WITNESS_CHILD            0x04    /* Child, aka direct descendant. */
#define	WITNESS_DESCENDANT       0x08    /* Direct or indirect descendant. */
#define	WITNESS_ANCESTOR_MASK    (WITNESS_PARENT | WITNESS_ANCESTOR)
#define	WITNESS_DESCENDANT_MASK  (WITNESS_CHILD | WITNESS_DESCENDANT)
#define	WITNESS_RELATED_MASK	 (WITNESS_ANCESTOR_MASK | WITNESS_DESCENDANT_MASK)
#define	WITNESS_REVERSAL         0x10    /* A lock order reversal has been observed. */
#define	WITNESS_RESERVED1        0x20    /* Unused flag, reserved. */
#define	WITNESS_RESERVED2        0x40    /* Unused flag, reserved. */
#define	WITNESS_LOCK_ORDER_KNOWN 0x80    /* This lock order is known. */

/*
 * Lock classes.  Each lock has a class which describes characteristics
 * common to all types of locks of a given class.
 *
 * Spin locks in general must always protect against preemption, as it is
 * an error to perform any type of context switch while holding a spin lock.
 * Also, for an individual lock to be recursable, its class must allow
 * recursion and the lock itself must explicitly allow recursion.
 */

struct lock_class {
	const	char 			*lc_name;
	u_int					lc_flags;
};

/*
struct lock_class {
	const		char *lc_name;
	u_int		lc_flags;
	void		(*lc_assert)(const struct lock_object *lock, int what);
	void		(*lc_ddb_show)(const struct lock_object *lock);
	void		(*lc_lock)(struct lock_object *lock, uintptr_t how);
	int			(*lc_owner)(const struct lock_object *lock, struct thread **owner);
	uintptr_t	(*lc_unlock)(struct lock_object *lock);
};
*/

union lock_stack {
	union lock_stack		*ls_next;
	struct stacktrace		ls_stack;
};


#define	LC_SLEEPLOCK	0x00000001	/* Sleep lock. */
#define	LC_SPINLOCK		0x00000002	/* Spin lock. */
#define	LC_SLEEPABLE	0x00000004	/* Sleeping allowed with this lock. */
#define	LC_RECURSABLE	0x00000008	/* Locks of this type may recurse. */
#define	LC_UPGRADABLE	0x00000010	/* Upgrades and downgrades permitted. */

/*
 * Lock instances.  A lock instance is the data associated with a lock while
 * it is held by witness.  For example, a lock instance will hold the
 * recursion count of a lock.  Lock instances are held in lists.  Spin locks
 * are held in a per-cpu list while sleep locks are held in per-thread list.
 */
struct lock_instance {
	struct lock_object		*li_lock;
	const char				*li_file;
	int						li_line;
	u_int					li_flags;
};

/*
 * A simple list type used to build the list of locks held by a thread
 * or CPU.  We can't simply embed the list in struct lock_object since a
 * lock may be held by more than one thread if it is a shared lock.  Locks
 * are added to the head of the list, so we fill up each list entry from
 * "the back" logically.  To ease some of the arithmetic, we actually fill
 * in each list entry the normal way (children[0] then children[1], etc.) but
 * when we traverse the list we read children[count-1] as the first entry
 * down to children[0] as the final entry.
 */
struct lock_list_entry {
	struct lock_list_entry	*ll_next;
	struct lock_instance	ll_children[LOCK_NCHILDREN];
	int						ll_count;
};

/*
 * The main witness structure. One of these per named lock type in the system
 * (for example, "vnode interlock").
 */
struct witness {
	const struct lock_type	*w_type;
	const char				*w_subtype;
	uint32_t				w_index;  			/* Index in the relationship matrix */
	struct lock_class		*w_class;
	SIMPLEQ_ENTRY(witness)	w_list;				/* List of all witnesses. */
	SIMPLEQ_ENTRY(witness)	w_typelist;			/* Witnesses of a type. */
	struct witness			*w_hash_next; 		/* Linked list in hash buckets. */
	uint16_t				w_num_ancestors; 	/* direct/indirect ancestor count */
	uint16_t				w_num_descendants; 	/* direct/indirect descendant count */
	int16_t					w_ddb_level;
	unsigned				w_acquired:1;
	unsigned				w_displayed:1;
	unsigned				w_reversed:1;
};

SIMPLEQ_HEAD(witness_list, witness);

/*
 * The witness hash table. Keys are witness names (const char *), elements are
 * witness objects (struct witness *).
 */
struct witness_hash {
	struct witness					*wh_array[WITNESS_HASH_SIZE];
	uint32_t						wh_size;
	uint32_t						wh_count;
};

/*
 * Key type for the lock order data hash table.
 */
struct witness_lock_order_key {
	uint16_t						from;
	uint16_t						to;
};

struct witness_lock_order_data {
	struct stacktrace				wlod_stack;
	struct witness_lock_order_key	wlod_key;
	struct witness_lock_order_data	*wlod_next;
};

/*
 * The witness lock order data hash table. Keys are witness index tuples
 * (struct witness_lock_order_key), elements are lock order data objects
 * (struct witness_lock_order_data).
 */
struct witness_lock_order_hash {
	struct witness_lock_order_data	*wloh_array[WITNESS_LO_HASH_SIZE];
	u_int							wloh_size;
	u_int							wloh_count;
};

struct witness_pendhelp {
	const struct lock_type			*wh_type;
	struct lock_object				*wh_lock;
};


struct witness_order_list_entry {
	const char						*w_name;
	struct lock_class				*w_class;
};

/*
 * Returns 0 if one of the locks is a spin lock and the other is not.
 * Returns 1 otherwise.
 */
static __inline int
witness_lock_type_equal(struct witness *w1, struct witness *w2)
{
	return ((w1->w_class->lc_flags & (LC_SLEEPLOCK | LC_SPINLOCK)) == (w2->w_class->lc_flags & (LC_SLEEPLOCK | LC_SPINLOCK)));
}

static __inline int
witness_lock_order_key_equal(const struct witness_lock_order_key *a,
    const struct witness_lock_order_key *b)
{
	return (a->from == b->from && a->to == b->to);
}

/*
 * If set to 0, lock order checking is disabled.  If set to -1,
 * witness is completely disabled.  Otherwise witness performs full
 * lock order checking for all locks.  At runtime, lock order checking
 * may be toggled.  However, witness cannot be reenabled once it is
 * completely disabled.
 */
#ifdef WITNESS_WATCH
static int witness_watch = 3;
#else
static int witness_watch = 2;
#endif

#ifdef WITNESS_LOCKTRACE
static int witness_locktrace = 1;
#else
static int witness_locktrace = 0;
#endif

int witness_count = WITNESS_COUNT;


/* lock list */
static struct lock_list_entry *w_lock_list_free = NULL;
static struct witness_pendhelp pending_locks[WITNESS_PENDLIST];
static u_int pending_cnt;


static struct lock_class lock_class_kernel_lock = {

};

static struct lock_class lock_class_sched_lock = {

};

static struct lock_class lock_class_mutex = {

};

static struct lock_class lock_class_rwlock = {

};

static struct lock_class lock_class_lock = {

};

static struct lock_class *lock_classes[] = {
	&lock_class_kernel_lock,
	&lock_class_sched_lock,
	&lock_class_mutex,
	&lock_class_rwlock,
	&lock_class_lock,
};

/*
 * This global is set to 0 once it becomes safe to use the witness code.
 */
static int witness_cold = 1;

/*
 * This global is set to 1 once the static lock orders have been enrolled
 * so that a warning can be issued for any spin locks enrolled later.
 */
static int witness_spin_warn = 0;

void
witness_init(struct lock_object *lock, const struct lock_type *type)
{
	struct lock_class *class;

	/* Various sanity checks. */
	class = LOCK_CLASS(lock);
	if ((lock->lock_flags & LO_RECURSABLE) != 0
			&& (class->lc_flags & LC_RECURSABLE) == 0)
		panic("%s: lock (%s) %s can not be recursable", __func__,
				class->lc_name, lock->lock_name);
	if ((lock->lock_flags & LO_SLEEPABLE) != 0
			&& (class->lc_flags & LC_SLEEPABLE) == 0)
		panic("%s: lock (%s) %s can not be sleepable", __func__, class->lc_name,
				lock->lock_name);
	if ((lock->lock_flags & LO_UPGRADABLE) != 0
			&& (class->lc_flags & LC_UPGRADABLE) == 0)
		panic("%s: lock (%s) %s can not be upgradable", __func__,
				class->lc_name, lock->lock_name);

	/*
	 * If we shouldn't watch this lock, then just clear lo_witness.
	 * Record the type in case the lock becomes watched later.
	 * Otherwise, if witness_cold is set, then it is too early to
	 * enroll this lock, so defer it to witness_initialize() by adding
	 * it to the pending_locks list.  If it is not too early, then enroll
	 * the lock now.
	 */
	if (witness_watch < 1 || panicstr != NULL || db_active
			|| (lock->lock_flags & LO_WITNESS) == 0) {
		lock->lock_witness = NULL;
		lock->lock_type = type;
	} else if (witness_cold) {
		pending_locks[pending_cnt].wh_lock = lock;
		pending_locks[pending_cnt++].wh_type = type;
		if (pending_cnt > WITNESS_PENDLIST)
			panic("%s: pending locks list is too small, "
					"increase WITNESS_PENDLIST", __func__);
	} else
		lock->lock_witness = enroll(type, lock->lock_name, class);
}
