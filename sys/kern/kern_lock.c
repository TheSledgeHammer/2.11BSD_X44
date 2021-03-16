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
 *	@(#)kern_lock.c	8.18 (Berkeley) 5/21/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/atomic.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <machine/cpu.h>

void	lock_pause(struct lock *, int);
void	lock_acquire(struct lock *, int, int, int);
void	count(struct proc *, short);

struct lock_holder *kernel_lockholder;

#if NCPUS > 1
#define PAUSE(lkp, wanted)						\
		lock_pause(lkp, wanted);
#else /* NCPUS == 1 */
#define PAUSE(lkp, wanted)
#endif /* NCPUS == 1 */

#define ACQUIRE(lkp, error, extflags, wanted)	\
		lock_acquire(lkp, error, extflags, wanted);

#ifdef DEBUG
#define COUNT(p, x) 							\
		count(p, x);
#else
#define COUNT(p, x)
#endif

/*
 * Locking primitives implementation.
 * Locks provide shared/exclusive sychronization.
 */

/* Initialize a lock; required before use. */
void
lockinit(lkp, prio, wmesg, timo, flags)
	struct lock *lkp;
	int prio;
	char *wmesg;
	int timo;
	int flags;
{
	bzero(lkp, sizeof(struct lock));
	simple_lock_init(&lkp->lk_lnterlock, "lock_init");
	lkp->lk_flags = flags & LK_EXTFLG_MASK;
	lkp->lk_prio = prio;
	lkp->lk_timo = timo;
	lkp->lk_wmesg = wmesg;

	/* lock holder */
	lkp->lk_lockholder = &kernel_lockholder;
	LOCKHOLDER_PID(lkp->lk_lockholder) = LK_NOPROC;
}

/* Determine the status of a lock. */
int
lockstatus(lkp)
	struct lock *lkp;
{
	int lock_type = 0;

	lkp_lock(lkp);
	if (lkp->lk_exclusivecount != 0) {
		lock_type = LK_EXCLUSIVE;
	} else if(lkp->lk_sharecount != 0) {
		lock_type = LK_SHARED;
	}
	lkp_unlock(lkp);
	return (lock_type);
}

/*
 * Set, change, or release a lock.
 *
 * Shared requests increment the shared count. Exclusive requests set the
 * LK_WANT_EXCL flag (preventing further shared locks), and wait for already
 * accepted shared locks and shared-to-exclusive upgrades to go away.
 */
int
lockmgr(lkp, flags, interlkp, pid)
	__volatile struct lock *lkp;
	u_int flags;
	struct lock_object *interlkp;
	pid_t pid;
{
	int error;
	int extflags;
	error = 0;

	if (!pid) {
		pid = LK_KERNPROC;
		LOCKHOLDER_PID(lkp->lk_lockholder) = LK_KERNPROC;
	}
	lkp_lock(&lkp);
	if (flags & LK_INTERLOCK) {
		lkp_unlock(&lkp);
	}
	extflags = (flags | lkp->lk_flags) & LK_EXTFLG_MASK;
#ifdef DIAGNOSTIC
	/*
	 * Once a lock has drained, the LK_DRAINING flag is set and an
	 * exclusive lock is returned. The only valid operation thereafter
	 * is a single release of that exclusive lock. This final release
	 * clears the LK_DRAINING flag and sets the LK_DRAINED flag. Any
	 * further requests of any sort will result in a panic. The bits
	 * selected for these two flags are chosen so that they will be set
	 * in memory that is freed (freed memory is filled with 0xdeadbeef).
	 * The final release is permitted to give a new lease on life to
	 * the lock by specifying LK_REENABLE.
	 */
	if (lkp->lk_flags & (LK_DRAINING|LK_DRAINED)) {
		if (lkp->lk_flags & LK_DRAINED) {
			panic("lockmgr: using decommissioned lock");
		}
		if ((flags & LK_TYPE_MASK) != LK_RELEASE || LOCKHOLDER_PID(lkp->lk_lockholder) != pid) {
			panic("lockmgr: non-release on draining lock: %d\n", flags & LK_TYPE_MASK);
		}
		lkp->lk_flags &= ~LK_DRAINING;
		if ((flags & LK_REENABLE) == 0) {
			lkp->lk_flags |= LK_DRAINED;
		}
	}
#endif DIAGNOSTIC

	switch (flags & LK_TYPE_MASK) {

	case LK_SHARED:
		if (LOCKHOLDER_PID(lkp->lk_lockholder) != pid) {
			/*
			 * If just polling, check to see if we will block.
			 */
			if ((extflags & LK_NOWAIT) && (lkp->lk_flags &
			    (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE))) {
				error = EBUSY;
				break;
			}
			/*
			 * Wait for exclusive locks and upgrades to clear.
			 */
			ACQUIRE(lkp, error, extflags, lkp->lk_flags & (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE));
			if (error)
				break;
			lkp->lk_sharecount++;
			//COUNT(pid, 1);
			break;
		}
		/*
		 * We hold an exclusive lock, so downgrade it to shared.
		 * An alternative would be to fail with EDEADLK.
		 */
		lkp->lk_sharecount++;
		//COUNT(pid, 1);
		/* fall into downgrade */
		break;

	case LK_DOWNGRADE:
		if (LOCKHOLDER_PID(lkp->lk_lockholder) != pid || lkp->lk_exclusivecount == 0)
			panic("lockmgr: not holding exclusive lock");
		lkp->lk_sharecount += lkp->lk_exclusivecount;
		lkp->lk_exclusivecount = 0;
		lkp->lk_flags &= ~LK_HAVE_EXCL;
		LOCKHOLDER_PID(lkp->lk_lockholder) = LK_NOPROC;
		if (lkp->lk_waitcount)
			wakeup((void *)lkp);
		break;

	case LK_EXCLUPGRADE:
		/*
		 * If another process is ahead of us to get an upgrade,
		 * then we want to fail rather than have an intervening
		 * exclusive access.
		 */
		if (lkp->lk_flags & LK_WANT_UPGRADE) {
			lkp->lk_sharecount--;
			//COUNT(pid, -1);
			error = EBUSY;
			break;
		}
		/* fall into normal upgrade */
		break;

	case LK_UPGRADE:
		/*
		 * Upgrade a shared lock to an exclusive one. If another
		 * shared lock has already requested an upgrade to an
		 * exclusive lock, our shared lock is released and an
		 * exclusive lock is requested (which will be granted
		 * after the upgrade). If we return an error, the file
		 * will always be unlocked.
		 */
		if (LOCKHOLDER_PID(lkp->lk_lockholder) == pid || lkp->lk_sharecount <= 0)
			panic("lockmgr: upgrade exclusive lock");
		lkp->lk_sharecount--;
		//COUNT(pid, -1);
		/*
		 * If we are just polling, check to see if we will block.
		 */
		if ((extflags & LK_NOWAIT) &&
		    ((lkp->lk_flags & LK_WANT_UPGRADE) ||
		     lkp->lk_sharecount > 1)) {
			error = EBUSY;
			break;
		}
		if ((lkp->lk_flags & LK_WANT_UPGRADE) == 0) {
			/*
			 * We are first shared lock to request an upgrade, so
			 * request upgrade and wait for the shared count to
			 * drop to zero, then take exclusive lock.
			 */
			lkp->lk_flags |= LK_WANT_UPGRADE;
			ACQUIRE(lkp, error, extflags, lkp->lk_sharecount);
			lkp->lk_flags &= ~LK_WANT_UPGRADE;
			if (error)
				break;
			lkp->lk_flags |= LK_HAVE_EXCL;
			LOCKHOLDER_PID(lkp->lk_lockholder) = pid;
			if (lkp->lk_exclusivecount != 0)
				panic("lockmgr: non-zero exclusive count");
			lkp->lk_exclusivecount = 1;
			//COUNT(pid, 1);
			break;
		}
		/*
		 * Someone else has requested upgrade. Release our shared
		 * lock, awaken upgrade requestor if we are the last shared
		 * lock, then request an exclusive lock.
		 */
		if (lkp->lk_sharecount == 0 && lkp->lk_waitcount)
			wakeup((void *)lkp);
		/* fall into exclusive request */
		break;

	case LK_EXCLUSIVE:
		if (LOCKHOLDER_PID(lkp->lk_lockholder) == pid && pid != LK_KERNPROC) {
			/*
			 *	Recursive lock.
			 */
			if ((extflags & LK_CANRECURSE) == 0)
				panic("lockmgr: locking against myself");
			lkp->lk_exclusivecount++;
			//COUNT(pid, 1);
			break;
		}
		/*
		 * If we are just polling, check to see if we will sleep.
		 */
		if ((extflags & LK_NOWAIT) && ((lkp->lk_flags &
				(LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) ||
				lkp->lk_sharecount != 0)) {
			error = EBUSY;
			break;
		}
		/*
		 * Try to acquire the want_exclusive flag.
		 */
		ACQUIRE(lkp, error, extflags, lkp->lk_flags &
				(LK_HAVE_EXCL | LK_WANT_EXCL));
		if (error)
			break;
		lkp->lk_flags |= LK_WANT_EXCL;
		/*
		 * Wait for shared locks and upgrades to finish.
		 */
		ACQUIRE(lkp, error, extflags, lkp->lk_sharecount != 0 ||
				(lkp->lk_flags & LK_WANT_UPGRADE));
		lkp->lk_flags &= ~LK_WANT_EXCL;
		if (error)
			break;
		lkp->lk_flags |= LK_HAVE_EXCL;
		LOCKHOLDER_PID(lkp->lk_lockholder) = pid;
		if (lkp->lk_exclusivecount != 0)
			panic("lockmgr: non-zero exclusive count");
		lkp->lk_exclusivecount = 1;
		//COUNT(pid, 1);
		break;

	case LK_RELEASE:
		if (lkp->lk_exclusivecount != 0) {
			if (pid != LOCKHOLDER_PID(lkp->lk_lockholder))
				panic("lockmgr: pid %d, not %s %d unlocking",
				    pid, "exclusive lock holder",
					LOCKHOLDER_PID(lkp->lk_lockholder));
			lkp->lk_exclusivecount--;
			COUNT(pid, -1);
			if (lkp->lk_exclusivecount == 0) {
				lkp->lk_flags &= ~LK_HAVE_EXCL;
				LOCKHOLDER_PID(lkp->lk_lockholder) = LK_NOPROC;
			}
		} else if (lkp->lk_sharecount != 0) {
			lkp->lk_sharecount--;
			//COUNT(pid, -1);
		}
		if (lkp->lk_waitcount)
			wakeup((void *)lkp);
		break;

	case LK_DRAIN:
		/*
		 * Check that we do not already hold the lock, as it can
		 * never drain if we do. Unfortunately, we have no way to
		 * check for holding a shared lock, but at least we can
		 * check for an exclusive one.
		 */
		if (LOCKHOLDER_PID(lkp->lk_lockholder) == pid)
			panic("lockmgr: draining against myself");
		/*
		 * If we are just polling, check to see if we will sleep.
		 */
		if ((extflags & LK_NOWAIT) && ((lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) ||
		     lkp->lk_sharecount != 0 || lkp->lk_waitcount != 0)) {
			error = EBUSY;
			break;
		}
		PAUSE(lkp, ((lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) ||
		     lkp->lk_sharecount != 0 || lkp->lk_waitcount != 0));
		for (error = 0; ((lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) ||
		     lkp->lk_sharecount != 0 || lkp->lk_waitcount != 0); ) {
			lkp->lk_flags |= LK_WAITDRAIN;
			lkp_unlock(&lkp);
			if (error == tsleep((void *)&lkp->lk_flags, lkp->lk_prio,
			    lkp->lk_wmesg, lkp->lk_timo))
				return (error);
			if ((extflags) & LK_SLEEPFAIL)
				return (ENOLCK);
			lkp_lock(&lkp);
		}
		lkp->lk_flags |= LK_DRAINING | LK_HAVE_EXCL;
		LOCKHOLDER_PID(lkp->lk_lockholder) = pid;
		lkp->lk_exclusivecount = 1;
		//COUNT(p, 1);
		break;

	default:
		lkp_unlock(lkp);
		panic("lockmgr: unknown locktype request %d", flags & LK_TYPE_MASK);
		/* NOTREACHED */
	}
	if ((lkp->lk_flags & LK_WAITDRAIN) && ((lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) == 0 &&
		     lkp->lk_sharecount == 0 && lkp->lk_waitcount == 0)) {
		lkp->lk_flags &= ~LK_WAITDRAIN;
		wakeup((void *)&lkp->lk_flags);
	}
	lkp_unlock(lkp);
	return (error);
}

/*
 * Print out information about state of a lock. Used by VOP_PRINT
 * routines to display ststus about contained locks.
 */
void
lockmgr_printinfo(lkp)
	struct lock *lkp;
{
	if (lkp->lk_sharecount)
		printf(" lock type %s: SHARED (count %d)", lkp->lk_wmesg,
		    lkp->lk_sharecount);
	else if (lkp->lk_flags & LK_HAVE_EXCL)
		printf(" lock type %s: EXCL (count %d) by pid %d",
		    lkp->lk_wmesg, lkp->lk_exclusivecount, lkp->lk_lockholder->lh_pid);
	if (lkp->lk_waitcount > 0)
		printf(" with %d pending", lkp->lk_waitcount);
}

int lock_wait_time = 100;
void
lock_pause(lkp, wanted)
	struct lock *lkp;
	int wanted;
{
	if (lock_wait_time > 0) {
		int i;
		lkp_unlock(lkp);
		for(i = lock_wait_time; i > 0; i--) {
			if (!(wanted)) {
				break;
			}
		}
		lkp_lock(lkp);
	}
	if (!(wanted)) {
		break;
	}
}

void
lock_acquire(lkp, error, extflags, wanted)
	struct lock *lkp;
	int error, extflags, wanted;
{
	lock_pause(lkp, wanted);
	for (error = 0; wanted; ) {
		lkp->lk_waitcount++;
		lkp_unlock(lkp);
		error = tsleep((void *)lkp, lkp->lk_prio, lkp->lk_wmesg, lkp->lk_timo);
		lkp_lock(lkp);
		lkp->lk_waitcount--;
		if (error) {
			break;
		}
		if ((extflags) & LK_SLEEPFAIL) {
			error = ENOLCK;
			break;
		}
	}
}

void
count(p, x)
	struct proc p;
	short x;
{
	if(p) {
		p->p_locks += x;
	}
}

/* Simple lock Interface: Compatibility with existing lock infrastructure */

#define lkp_lock(lkp) 		\
		simple_lock((lkp)->lk_lnterlock);

#define lkp_unlock(lkp) 	\
		simple_unlock((lkp)->lk_lnterlock);

#define lkp_lock_try(lkp) 	\
		simple_lock_try((lkp)->lk_lnterlock);

/* simple_lock_init */
void
simple_lock_init(lock, name)
	struct lock_object 		*lock;
	const char				*name;
{
	lock_object_init(lock, NULL, name, NULL);
}

/*
 * simple_lock
 * lock_object lock interface
 */
void
simple_lock(lock)
	__volatile struct lock_object 	*lock;
{
	lock_object_acquire(&lock);
}

/*
 * simple_unlock
 * lock_object unlock interface
 */
void
simple_unlock(lock)
	__volatile struct lock_object 	*lock;
{
	lock_object_release(&lock);
}

/* simple_lock_try */
int
simple_lock_try(lock)
	__volatile struct lock_object 	*lock;
{
	return (lock_object_try(&lock));
}

/*
 * [Internal Use Only]:
 * Lock-Object: (Simple-Lock Replacement)
 * An Array-Based-Queuing-Lock.
 */
void
lock_object_init(lock, type, name, flags)
	struct lock_object 		*lock;
	const struct lock_type 	*type;
    const char				*name;
    u_int					flags;
{
    memset(lock->lo_cpus, 0, sizeof(lock->lo_cpus));

    lock->lo_nxt_ticket = 0;
    for (int i = 1; i < cpu_number; i++) {
    	lock->lo_can_serve[i] = 0;
    }
    lock->lo_can_serve[0] = 1;

	lock->lo_name = name;
	lock->lo_flags = flags;
}

/*
 * [Internal Use Only]:
 * Acquire the lock
 */
void
lock_object_acquire(lock)
	struct lock_object 	*lock;
{
	struct lock_object_cpu *cpu = &lock->lo_cpus[cpu_number];
	unsigned long s;

	s = intr_disable();
	cpu->loc_my_ticket = atomic_inc_int_nv(lock->lo_nxt_ticket);
	while(lock->lo_can_serve[&cpu->loc_my_ticket] != 1);
	intr_restore(s);
}

/*
 * [Internal Use Only]:
 * Release the lock
 */
void
lock_object_release(lock)
	struct lock_object 	*lock;
{
	struct lock_object_cpu *cpu = &lock->lo_cpus[cpu_number];
	unsigned long s;

	s = intr_disable();
	lock->lo_can_serve[&cpu->loc_my_ticket + 1] = 1;
	lock->lo_can_serve[&cpu->loc_my_ticket] = 0;
	intr_restore(s);
}

/*
 * [Internal Use Only]:
 * Try to obtain the lock
 */
int
lock_object_try(lock)
	struct lock_object 	*lock;
{
	struct lock_object_cpu *cpu = &lock->lo_cpus[cpu_number];

	return (!lock->lo_can_serve[&cpu->loc_my_ticket]);
}

/*
 * Lock Holder:
 */
//struct lock_holder *kernel_lockholder;

static void
lockholder_alloc(holder)
	struct lock_holder *holder;
{
	memset(holder, 0, sizeof(struct lock_holder));
	//rmalloc(holder, sizeof(struct lock_holder));
}

void
lockholder_init(proc)
	struct proc *proc;
{
	lockholder_alloc(&kernel_lockholder);
	set_proc_lockholder(&kernel_lockholder, proc);
}

void
set_proc_lockholder(holder, proc)
	struct lock_holder *holder;
	struct proc *proc;
{
	holder->lh_pid = proc->p_pid;
	holder->lh_pgrp = proc->p_pgrp;
	PROC_LOCKHOLDER(holder) = proc;
}

struct proc *
get_proc_lockholder(holder)
	struct lock_holder 	*holder;
{
	if(PROC_LOCKHOLDER(holder)->p_pid == holder->lh_pid) {
		return (PROC_LOCKHOLDER(holder));
	}
	return (NULL);
}

/*
void
set_kthread_lockholder(holder, kthread)
	struct lock_holder 	*holder;
	struct kthread 		*kthread;
{
	holder->lh_pid = kthread->kt_tid;
	holder->lh_pgrp = kthread->kt_pgrp;
	KTHREAD_LOCKHOLDER(holder) = kthread;
}

struct kthread *
get_kthread_lockholder(holder)
	struct lock_holder 	*holder;
{
	if(KTHREAD_LOCKHOLDER(holder)->kt_tid == holder->lh_pid) {
		return (KTHREAD_LOCKHOLDER(holder));
	}
	return (NULL);
}

void
set_uthread_lockholder(holder, uthread)
	struct lock_holder 	*holder;
	struct uthread 		*uthread;
{
	holder->lh_pid = uthread->ut_tid;
	holder->lh_pgrp = uthread->ut_pgrp;
	UTHREAD_LOCKHOLDER(holder) = uthread;
}

struct uthread *
get_uthread_lockholder(holder)
	struct lock_holder 	*holder;
{
	if(UTHREAD_LOCKHOLDER(holder)->ut_tid == holder->lh_pid) {
		return (UTHREAD_LOCKHOLDER(holder));
	}
	return (NULL);
}
*/
