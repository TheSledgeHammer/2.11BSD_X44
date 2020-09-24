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

//#include <sys/lock.h>
#include <devel/sys/lockobj.h>
#include <devel/sys/witness.h>
#include <sys/user.h>

struct lock kernel_lock;
struct lock sched_lock;
struct lock rwlock;
struct lock simlock;

void
kernel_lock_init(prio, wmesg, timo, flags)
	int prio;
	char *wmesg;
	int timo;
	int flags;
{
	lockinit(&kernel_lock, prio, wmesg, timo, flags);
	__lockwitness(&kernel_lock);
}

void
lockwitness(struct lock *lkp, const struct lock_type *type)
{
#ifdef WITNESS
	lkp->lk_lockobject.lock_name = type->lt_name;
	lkp->lk_lockobject.lock_type = type;
	if (lkp == &kernel_lock) {
		lkp->lk_lockobject.lock_flags = LO_WITNESS | LO_INITIALIZED | LO_SLEEPABLE | (LO_CLASS_KERNEL_LOCK << LO_CLASSSHIFT);
	} else if (lkp == &sched_lock) {
		lkp->lk_lockobject.lock_flags = LO_WITNESS | LO_INITIALIZED | LO_RECURSABLE | (LO_CLASS_SCHED_LOCK  << LO_CLASSSHIFT);
	}
	WITNESS_INIT(&lkp->lk_lockobject, type);
#endif
}

/* Sets kthread to lockholder if not null */
void
set_kthread_lock(lkp, kt)
	struct lock *lkp;
	struct kthread *kt;
{
	if(kt == NULL) {
		lkp->lk_ktlockholder = NULL;
	} else {
		lkp->lk_ktlockholder = kt;
	}
}

/* Sets uthread to lockholder if not null */
void
set_uthread_lock(lkp, ut)
	struct lock *lkp;
	struct uthread *ut;
{
	if(ut == NULL) {
		lkp->lk_utlockholder = NULL;
	} else {
		lkp->lk_utlockholder = ut;
	}
}

/* Returns kthread if pid matches and lockholder is not null */
struct kthread *
get_kthread_lock(lkp, pid)
	struct lock *lkp;
	pid_t pid;
{
	struct kthread *klk =  lkp->lk_ktlockholder;
	if(klk != NULL && klk->kt_tid == pid) {
		return (klk);
	}
	return (NULL);
}

/* Returns uthread if pid matches and lockholder is not null */
struct uthread *
get_uthread_lock(lkp, pid)
	struct lock *lkp;
	pid_t pid;
{
	struct uthread *ulk = lkp->lk_utlockholder;
	if(ulk != NULL && ulk->ut_tid == pid) {
		return (ulk);
	}
	return (NULL);
}

#if defined(DEBUG) && NCPUS == 1

#include <sys/kernel.h>
#include <vm/include/vm.h>
#include <sys/sysctl.h>

int lockpausetime = 0;
struct ctldebug debug2 = { "lockpausetime", &lockpausetime };
int simplelockrecurse;

/* Example in OpenBSD kern_lock.c */
/* below are all re-implemented using a lock object instead of simplelock */

/*
 * Simple lock functions so that the debugger can see from whence
 * they are being called.
 */
void
simple_lock_init(alp)
	struct lock_object *alp;
{
	alp->lock_data = 0;
}

void
_simple_lock(alp, id, l)
	__volatile struct lock_object *alp;
	const char *id;
	int l;
{
	if (simplelockrecurse)
		return;
	if (alp->lock_data == 1) {
		if (lockpausetime == -1)
			panic("%s:%d: simple_lock: lock held", id, l);
		printf("%s:%d: simple_lock: lock held\n", id, l);
		if (lockpausetime == 1) {
			BACKTRACE(curproc);
		} else if (lockpausetime > 1) {
			printf("%s:%d: simple_lock: lock held...", id, l);
			tsleep(&lockpausetime, PCATCH | PPAUSE, "slock", lockpausetime * hz);
			printf(" continuing\n");
		}
	}
	alp->lock_data = 1;
	if (curproc)
		curproc->p_simple_locks++;
}

int
_simple_lock_try(alp, id, l)
	__volatile struct lock_object *alp;
	const char *id;
	int l;
{
	if (alp->lock_data)
		return (0);
	if (simplelockrecurse)
		return (1);
	alp->lock_data = 1;
	if (curproc)
		curproc->p_simple_locks++;
	return (1);
}

void
_simple_unlock(alp, id, l)
	__volatile struct lock_object *alp;
	const char *id;
	int l;
{

	if (simplelockrecurse)
		return;
	if (alp->lock_data == 0) {
		if (lockpausetime == -1)
			panic("%s:%d: simple_unlock: lock not held", id, l);
		printf("%s:%d: simple_unlock: lock not held\n", id, l);
		if (lockpausetime == 1) {
			BACKTRACE(curproc);
		} else if (lockpausetime > 1) {
			printf("%s:%d: simple_unlock: lock not held...", id, l);
			tsleep(&lockpausetime, PCATCH | PPAUSE, "sunlock", lockpausetime * hz);
			printf(" continuing\n");
		}
	}
	alp->lock_data = 0;
	if (curproc)
		curproc->p_simple_locks--;
}

#endif /* DEBUG && NCPUS == 1 */
