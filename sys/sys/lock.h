/*
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code contains ideas from software contributed to Berkeley by
 * Avadis Tevanian, Jr., Michael Wayne Young, and the Mach Operating
 * System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)lock.h	8.12 (Berkeley) 5/19/95
 */

#ifndef	_SYS_LOCK_H_
#define	_SYS_LOCK_H_

#include <machine/param.h>
#include <machine/lock_machdep.h>

/*
 * common lock object:
 * array based queuing lock (ABQL)
 */
struct lock_object_cpu {
	__cpu_simple_lock_t			loc_my_ticket;			/* current ticket */
};

struct lock_object {
	struct lock_object_cpu	 	lo_cpu_data[NCPUS];		/* lock array */
	struct lock_object_cpu		*lo_next;				/* next lock in array */
	struct lock_object_cpu		*lo_prev;				/* previous lock in array */
	__cpu_simple_lock_t			lo_next_ticket;			/* next ticket */
	__cpu_simple_lock_t			lo_prev_ticket;			/* previous ticket */
	int							lo_can_serve[NCPUS];	/* can serve */

	const struct lock_type		*lo_type;				/* type. unused */
	const char 					*lo_name;				/* individual lock name. */
	u_int						lo_flags;				/* flags. Unused */
	//struct witness 				*lo_witness;		/* data for witness. Unused */
};

struct lock_type {
	const char					*lt_name;
};

/* lock holder of current lock */
struct lock_holder {
	pid_t						lh_pid;				/* pid of current lock holder */
	struct pgrp 				*lh_pgrp;			/* pgrp of current lock holder */
	void						*lh_data;			/* data of the current lock holder */
};

/*
 * The general lock structure.  Provides for multiple shared locks,
 * upgrading from shared to exclusive, and sleeping until the lock
 * can be gained. The simple locks are defined in <machine/param.h>.
 */
struct lock {
	struct  lock_object			lk_lnterlock;		/* lock object */
	struct  lock_holder			lk_lockholder;		/* lock holder */

    int							lk_sharecount;		/* # of accepted shared locks */
    int							lk_waitcount;		/* # of processes sleeping for lock */
    short						lk_exclusivecount;	/* # of recursive exclusive locks */

    u_int						lk_flags;			/* see below */
    short						lk_prio;			/* priority at which to sleep */
    char						*lk_wmesg;			/* resource sleeping (for tsleep) */
    int							lk_timo;			/* maximum sleep time (for tsleep) */
};
typedef struct lock       	*lock_t;

/*
 * Lock request types:
 *   LK_SHARED - get one of many possible shared locks. If a process
 *	holding an exclusive lock requests a shared lock, the exclusive
 *	lock(s) will be downgraded to shared locks.
 *   LK_EXCLUSIVE - stop further shared locks, when they are cleared,
 *	grant a pending upgrade if it exists, then grant an exclusive
 *	lock. Only one exclusive lock may exist at a time, except that
 *	a process holding an exclusive lock may get additional exclusive
 *	locks if it explicitly sets the LK_CANRECURSE flag in the lock
 *	request, or if the LK_CANRECUSE flag was set when the lock was
 *	initialized.
 *   LK_UPGRADE - the process must hold a shared lock that it wants to
 *	have upgraded to an exclusive lock. Other processes may get
 *	exclusive access to the resource between the time that the upgrade
 *	is requested and the time that it is granted.
 *   LK_EXCLUPGRADE - the process must hold a shared lock that it wants to
 *	have upgraded to an exclusive lock. If the request succeeds, no
 *	other processes will have gotten exclusive access to the resource
 *	between the time that the upgrade is requested and the time that
 *	it is granted. However, if another process has already requested
 *	an upgrade, the request will fail (see error returns below).
 *   LK_DOWNGRADE - the process must hold an exclusive lock that it wants
 *	to have downgraded to a shared lock. If the process holds multiple
 *	(recursive) exclusive locks, they will all be downgraded to shared
 *	locks.
 *   LK_RELEASE - release one instance of a lock.
 *   LK_DRAIN - wait for all activity on the lock to end, then mark it
 *	decommissioned. This feature is used before freeing a lock that
 *	is part of a piece of memory that is about to be freed.
 *
 * These are flags that are passed to the lockmgr routine.
 */
#define LK_TYPE_MASK	0x0000000f	/* type of lock sought */
#define LK_SHARED		0x00000001	/* shared lock */
#define LK_EXCLUSIVE	0x00000002	/* exclusive lock */
#define LK_UPGRADE		0x00000003	/* shared-to-exclusive upgrade */
#define LK_EXCLUPGRADE	0x00000004	/* first shared-to-exclusive upgrade */
#define LK_DOWNGRADE	0x00000005	/* exclusive-to-shared downgrade */
#define LK_RELEASE		0x00000006	/* release any type of lock */
#define LK_DRAIN		0x00000007	/* wait for all lock activity to end */
/*
 * External lock flags.
 *
 * The first three flags may be set in lock_init to set their mode permanently,
 * or passed in as arguments to the lock manager. The LK_REENABLE flag may be
 * set only at the release of a lock obtained by drain.
 */
#define LK_EXTFLG_MASK	0x00000070	/* mask of external flags */
#define LK_NOWAIT		0x00000010	/* do not sleep to await lock */
#define LK_SLEEPFAIL	0x00000020	/* sleep, then return failure */
#define LK_CANRECURSE	0x00000040	/* allow recursive exclusive lock */
#define LK_REENABLE		0x00000080	/* lock is be reenabled after drain */
/*
 * Internal lock flags.
 *
 * These flags are used internally to the lock manager.
 */
#define LK_WANT_UPGRADE	0x00000100	/* waiting for share-to-excl upgrade */
#define LK_WANT_EXCL	0x00000200	/* exclusive lock sought */
#define LK_HAVE_EXCL	0x00000400	/* exclusive lock obtained */
#define LK_WAITDRAIN	0x00000800	/* process waiting for lock to drain */
#define LK_DRAINING		0x00004000	/* lock is being drained */
#define LK_DRAINED		0x00008000	/* lock has been decommissioned */
/*
 * Control flags
 *
 * Non-persistent external flags.
 */
#define LK_INTERLOCK	0x00010000	/* unlock passed simple lock after getting lk_interlock */
#define LK_RETRY		0x00020000	/* vn_lock: retry until locked */

/*
 * Lock return status.
 *
 * Successfully obtained locks return 0. Locks will always succeed
 * unless one of the following is true:
 *	LK_FORCEUPGRADE is requested and some other process has already
 *	    requested a lock upgrade (returns EBUSY).
 *	LK_WAIT is set and a sleep would be required (returns EBUSY).
 *	LK_SLEEPFAIL is set and a sleep was done (returns ENOLCK).
 *	PCATCH is set in lock priority and a signal arrives (returns
 *	    either EINTR or ERESTART if system calls is to be restarted).
 *	Non-null lock timeout and timeout expires (returns EWOULDBLOCK).
 * A failed lock attempt always returns a non-zero error value. No lock
 * is held after an error return (in particular, a failed LK_UPGRADE
 * or LK_FORCEUPGRADE will have released its shared access lock).
 */

/*
 * Indicator that no process holds exclusive lock
 */
#define LK_KERNPROC 			((pid_t) -2)
#define LK_NOPROC 				((pid_t) -1)

/*
 * lock holder macros
 */
#define LOCKHOLDER_PID(h)		((h)->lh_pid)
#define LOCKHOLDER_PGRP(h)		((h)->lh_pgrp)
#define LOCKHOLDER_DATA(h)		((h)->lh_data)

#define LOCKHOLDER_PROC(h) 		((struct proc *)LOCKHOLDER_DATA(h))
//#define LOCKHOLDER_KTHREAD(h) 	((struct kthread *)LOCKHOLDER_DATA(h))
//#define LOCKHOLDER_UTHREAD(h) 	((struct uthread *)LOCKHOLDER_DATA(h))

#ifdef _KERNEL
struct pgrp;

void				lockinit(struct lock *, int, char *, int, int);
int				lockmgr(__volatile struct lock *, u_int, struct lock_object *, pid_t);
int				lockstatus(struct lock *);
void				lockmgr_printinfo(struct lock *);

void				simple_lock_init(struct lock_object *, const char *);
void 				simple_lock(__volatile struct lock_object *);
void 				simple_unlock(__volatile struct lock_object *);
int				simple_lock_try(__volatile struct lock_object *);

void				lockholder_init(struct lock_holder *);
struct lock_holder 		*lockholder_create(void *, pid_t, struct pgrp *);
void				lockholder_set(struct lock_holder *, void *, pid_t, struct pgrp *);
struct lock_holder		*lockholder_get(struct lock_holder *);

#endif /* _KERNEL */
#endif /* !_SYS_LOCK_H_ */
