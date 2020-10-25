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
/* parts are to be merged into kern_lock.c */

#include <sys/lock.h>
#include <devel/sys/lockobj.h>
#include <devel/sys/witness.h>
#include <sys/user.h>

/* lock class */
struct lock_class lock_class_lock = {
		.lc_name = "lock",
		.lc_flags = LC_SLEEPLOCK | LC_SLEEPABLE | LC_UPGRADABLE,
		.lc_lock = lockoject_lock,
		.lc_unlock = lockoject_unlock
};

void
lockobject_init(lock, type, name, flags)
	struct lock_object 		*lock;
	const struct lock_type 	*type;
    const char				*name;
    u_int					flags;
{
	lock->lo_data = 0;
	lock->lo_name = name;
	lock->lo_flags = flags;
	lock->lo_type = type;
    type->lt_name = name;
	WITNESS_INIT(&lock, type);
}

void
lockoject_lock(lock, name)
	struct lock_object 	*lock;
	const char			*name;
{
	if(lock->lo_name == name) {
		WITNESS_LOCK(&lock, lock->lo_flags);
	}
}

void
lockoject_unlock(lock, name)
	struct lock_object 	*lock;
	const char			*name;
{
	if(lock->lo_name == name) {
		WITNESS_UNLOCK(&lock, lock->lo_flags);
	}
}

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
set_proc_lockholder(holder, proc)
	struct lock_holder 	*holder;
	struct proc 		*proc;
{
	holder->lh_pid = proc->p_pid;
	PROC_LOCKHOLDER(holder) = proc;
}

void
set_kthread_lockholder(holder, kthread)
	struct lock_holder 	*holder;
	struct kthread 		*kthread;
{
	holder->lh_pid = kthread->kt_tid;
	KTHREAD_LOCKHOLDER(holder) = kthread;
}

void
set_uthread_lockholder(holder, uthread)
	struct lock_holder 	*holder;
	struct uthread 		*uthread;
{
	holder->lh_pid = uthread->ut_tid;
	UTHREAD_LOCKHOLDER(holder) = uthread;
}

struct proc *
get_proc_lockholder(holder, pid)
	struct lock_holder 	*holder;
	pid_t pid;
{
	if(PROC_LOCKHOLDER(holder)->p_pid == holder->lh_pid && holder->lh_pid == pid) {
		return (PROC_LOCKHOLDER(holder));
	}
	return (NULL);
}

struct kthread *
get_kthread_lockholder(holder, pid)
	struct lock_holder 	*holder;
	pid_t pid;
{
	if(KTHREAD_LOCKHOLDER(holder)->kt_tid == holder->lh_pid && holder->lh_pid == pid) {
		return (KTHREAD_LOCKHOLDER(holder));
	}
	return (NULL);
}

struct uthread *
get_uthread_lockholder(holder, pid)
	struct lock_holder 	*holder;
	pid_t pid;
{
	if(UTHREAD_LOCKHOLDER(holder)->ut_tid == holder->lh_pid && holder->lh_pid == pid) {
		return (UTHREAD_LOCKHOLDER(holder));
	}
	return (NULL);
}
