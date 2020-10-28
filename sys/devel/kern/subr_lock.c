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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <devel/sys/atomic.h>
#include <devel/sys/lockobj.h>
#include <devel/sys/rwlock.h>
#include <devel/sys/kthread.h>
#include <devel/sys/witness.h>


/* Simple lock Interface: Compatibility with the lock_object API */
void
simple_lock_init(lock, type, name, flags)
	struct lock_object 		*lock;
	const struct lock_type 	*type;
	const char				*name;
	u_int					flags;
{
	lock_object_init(lock, type, name, flags);
}

/* lock_object lock interface */
void
simple_lock(lock)
	struct lock_object 	*lock;
{
	lock_object_acquire(lock);
}

/* lock_object unlock interface */
void
simple_unlock(lock)
	struct lock_object 	*lock;
{
	lock_object_release(lock);
}

/*
 * [Internal Use Only (for the time being)]:
 * Lock-Object: New simple_lock replacement.
 * An Array-Based-Queuing-Lock, with Atomic operations.
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
    for (int i = 1; i < cpu_number(); i++) {
    	lock->lo_can_serve[i] = 0;
    }
    lock->lo_can_serve[0] = 1;

	lock->lo_name = name;
	lock->lo_flags = LO_INITIALIZED | flags;

#ifdef WITNESS
	lock->lo_type = type;
    type->lt_name = name;
	WITNESS_INIT(&lock, type);
#endif
}

/*
 * [Internal Use Only]:
 * Acquire the lock
 */
void
lock_object_acquire(lock)
	struct lock_object 	*lock;
{
	struct lock_object_cpu *cpu = &lock->lo_cpus[cpu_number()];
	unsigned long s;

#ifdef WITNESS
	if (!cpu->loc_my_ticket == lock->lo_nxt_ticket)
			WITNESS_CHECKORDER(&lock, LOP_EXCLUSIVE | LOP_NEWORDER, NULL);
#endif

	s = intr_disable();
	cpu->loc_my_ticket = atomic_inc_int_nv(lock->lo_nxt_ticket);
	while(lock->lo_can_serve[&cpu->loc_my_ticket] != 1);
	intr_restore(s);

#ifdef WITNESS
	WITNESS_LOCK(&lock, LOP_EXCLUSIVE);
#endif
}

/*
 * [Internal Use Only]:
 * Release the lock
 */
void
lock_object_release(lock)
	struct lock_object 	*lock;
{
	struct lock_object_cpu *cpu = &lock->lo_cpus[cpu_number()];
	unsigned long s;

#ifdef WITNESS
	WITNESS_UNLOCK(&lock, LOP_EXCLUSIVE);
#endif

	s = intr_disable();
	lock->lo_can_serve[&cpu->loc_my_ticket + 1] = 1;
	lock->lo_can_serve[&cpu->loc_my_ticket] = 0;
	intr_restore(s);
}


/*
 * Lock Holder:
 */
void
lock_lockholder_init(lkp)
	struct lock *lkp;
{
	bzero(lkp->lk_lockholder, sizeof(struct lock_holder));
	lkp->lk_lockholder->lh_pid = LK_NOPROC;

	set_proc_lockholder(lkp->lk_lockholder, NULL);
    set_kthread_lockholder(lkp->lk_lockholder, NULL);
	set_uthread_lockholder(lkp->lk_lockholder, NULL);
}

void
rwlock_lockholder_init(rwl)
	struct rwlock *rwl;
{
	bzero(rwl->rwl_lockholder, sizeof(struct lock_holder));
	rwl->rwl_lockholder->lh_pid = RW_NOTHREAD;

	set_proc_lockholder(rwl->rwl_lockholder, NULL);
    set_kthread_lockholder(rwl->rwl_lockholder, NULL);
	set_uthread_lockholder(rwl->rwl_lockholder, NULL);
}

void
set_proc_lockholder(holder, proc)
	struct lock_holder 	*holder;
	struct proc 		*proc;
{
	holder->lh_pid = proc->p_pid;
	holder->lh_pgrp = proc->p_pgrp;
	PROC_LOCKHOLDER(holder) = proc;
}

void
set_kthread_lockholder(holder, kthread)
	struct lock_holder 	*holder;
	struct kthread 		*kthread;
{
	holder->lh_pid = kthread->kt_tid;
	holder->lh_pgrp = kthread->kt_pgrp;
	KTHREAD_LOCKHOLDER(holder) = kthread;
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

struct proc *
get_proc_lockholder(holder)
	struct lock_holder 	*holder;
{
	if(PROC_LOCKHOLDER(holder)->p_pid == holder->lh_pid) {
		return (PROC_LOCKHOLDER(holder));
	}
	return (NULL);
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

struct uthread *
get_uthread_lockholder(holder)
	struct lock_holder 	*holder;
{
	if(UTHREAD_LOCKHOLDER(holder)->ut_tid == holder->lh_pid) {
		return (UTHREAD_LOCKHOLDER(holder));
	}
	return (NULL);
}
