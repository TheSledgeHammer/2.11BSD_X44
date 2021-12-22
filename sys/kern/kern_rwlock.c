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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/lock.h>
#include <sys/rwlock.h>

#include <machine/cpu.h>

void		rwlock_pause(rwlock_t, int);
void		rwlock_acquire(rwlock_t, int, int, int);

#if NCPUS > 1
#define PAUSE(rwl, wanted)						\
		rwlock_pause(rwl, wanted);
#else /* NCPUS == 1 */
#define PAUSE(rwl, wanted)
#endif /* NCPUS == 1 */

#define ACQUIRE(rwl, error, extflags, wanted)	\
		rwlock_acquire(rwl, error, extflags, wanted);

/* Initialize a rwlock */
void
rwlock_init(rwl, prio, wmesg, timo, flags)
	rwlock_t rwl;
	char *wmesg;
	int prio, timo;
	unsigned int flags;
{
	bzero(rwl, sizeof(struct rwlock));
	simple_lock_init(&rwl->rwl_lnterlock, "rwlock_init");

	rwl->rwl_flags = flags & RW_EXTFLG_MASK;
	rwl->rwl_prio = prio;
	rwl->rwl_timo = timo;
	rwl->rwl_wmesg = wmesg;

	/* lock holder */
	lockholder_init(rwl->rwl_lockholder);
}

int
rwlockstatus(rwl)
	rwlock_t rwl;
{
    int lock_type = 0;
    rwlock_lock(rwl);
    if(rwl->rwl_writercount >= 1) {
        lock_type = RW_WRITER;
    } else if(rwl->rwl_readercount >= 0) {
        lock_type = RW_READER;
    }
    rwlock_unlock(rwl);
    return (lock_type);
}

int
rwlockmgr(rwl, flags, interlkp, pid)
	__volatile rwlock_t rwl;
	unsigned int 		flags;
	struct lock_object *interlkp;
	pid_t 				pid;
{
	int error;
	int extflags;
	error = 0;

	if (!pid) {
		pid = RW_KERNPROC;
	}
	LOCKHOLDER_PID(rwl->rwl_lockholder) = pid;
	rwlock_lock(&rwl);
	if (flags & RW_INTERLOCK) {
		rwlock_unlock(&rwl);
	}
	extflags = (flags | rwl->rwl_flags) & RW_EXTFLG_MASK;

	switch (flags & RW_TYPE_MASK) {
	case RW_WRITER:
		if (LOCKHOLDER_PID(rwl->rwl_lockholder) != pid) {
			if ((extflags & RW_NOWAIT)
					&& (rwl->rwl_flags & (RW_HAVE_READ | RW_WANT_READ | RW_WANT_DOWNGRADE))) {
				error = EBUSY;
				break;
			}
			ACQUIRE(rwl, error, extflags, rwl->rwl_flags & (RW_HAVE_READ | RW_WANT_READ | RW_WANT_DOWNGRADE));
			if (error)
				break;
			rwl->rwl_writercount++;
			break;
		}
		rwl->rwl_writercount++;

	case RW_DOWNGRADE:
		if (LOCKHOLDER_PID(rwl->rwl_lockholder) != pid || rwl->rwl_readercount == 0)
			panic("rwlockmgr: not holding reader lock");
		rwl->rwl_writercount += rwl->rwl_readercount;
		rwl->rwl_readercount = 0;
		rwl->rwl_flags &= ~RW_HAVE_READ;
		LOCKHOLDER_PID(rwl->rwl_lockholder) = RW_NOPROC;
		if (rwl->rwl_waitcount)
			wakeup((void *)rwl);
		break;

	case RW_READER:
		if (LOCKHOLDER_PID(rwl->rwl_lockholder) != pid) {
			if ((extflags & RW_WAIT) && (rwl->rwl_flags & (RW_HAVE_WRITE | RW_WANT_WRITE | RW_WANT_UPGRADE))) {
				error = EBUSY;
				break;
			}
			ACQUIRE(rwl, error, extflags, rwl->rwl_flags & (RW_HAVE_WRITE | RW_WANT_WRITE | RW_WANT_UPGRADE));
			if (error)
				break;
			rwl->rwl_readercount++;
			break;
		}
		rwl->rwl_readercount++;

	case RW_UPGRADE:
		if (LOCKHOLDER_PID(rwl->rwl_lockholder) != pid || rwl->rwl_writercount < 1)
			panic("rwlockmgr: not holding writer lock");
		rwl->rwl_readercount += rwl->rwl_writercount;
		rwl->rwl_writercount = 1;
		rwl->rwl_flags &= ~RW_HAVE_WRITE;
		LOCKHOLDER_PID(rwl->rwl_lockholder) = RW_NOPROC;
		if (rwl->rwl_waitcount)
			wakeup((void *)rwl);
		break;

	default:
		rwlock_unlock(&rwl);
		printf("rwlockmgr: unknown locktype request %d", flags & RW_TYPE_MASK); /* panic */
		break;
	}

	rwlock_unlock(&rwl);
	return (error);
}

/*
 * Print out information about state of a lock. Used by VOP_PRINT
 * routines to display status about contained locks.
 */
void
rwlockmgr_printinfo(rwl)
	struct rwlock *rwl;
{
	if (rwl->rwl_writercount)
		printf(" lock type %s: RW_WRITER (count %d)", rwl->rwl_wmesg, rwl->rwl_writercount);
	if (rwl->rwl_readercount)
		printf(" lock type %s: RW_READER (count %d)", rwl->rwl_wmesg, rwl->rwl_readercount);
}

/*
 * rw_read_held:
 *
 *	Returns true if the rwlock is held for reading.  Must only be
 *	used for diagnostic assertions, and never be used to make
 * 	decisions about how to use a rwlock.
 */
int
rwlock_read_held(rwl)
	__volatile rwlock_t rwl;
{
	register struct lock_object *lock = rwl->rwl_lnterlock;
	register struct lock_object_cpu *cpu = lock->lo_cpus[cpu_number()];
	if (rwl == NULL)
		return 0;
	return (cpu->loc_my_ticket & RW_HAVE_WRITE) == 0 && (cpu->loc_my_ticket & RW_KERNPROC) != 0;
}

/*
 * rw_write_held:
 *
 *	Returns true if the rwlock is held for writing.  Must only be
 *	used for diagnostic assertions, and never be used to make
 *	decisions about how to use a rwlock.
 */
int
rwlock_write_held(rwl)
	__volatile rwlock_t rwl;
{
	register struct lock_object *lock = &rwl->rwl_lnterlock;
	register struct lock_object_cpu *cpu = &lock->lo_cpus[cpu_number()];
	if (rwl == NULL)
		return (0);
	return (cpu->loc_my_ticket & (RW_HAVE_WRITE | RW_KERNPROC) == (RW_HAVE_WRITE | curproc));
}

/*
 * rw_lock_held:
 *
 *	Returns true if the rwlock is held for reading or writing.  Must
 *	only be used for diagnostic assertions, and never be used to make
 *	decisions about how to use a rwlock.
 */
int
rwlock_lock_held(rwl)
	__volatile rwlock_t rwl;
{
	register struct lock_object *lock = &rwl->rwl_lnterlock;
	register struct lock_object_cpu *cpu = &lock->lo_cpus[cpu_number()];
	if (rwl == NULL)
		return 0;
	return (cpu->loc_my_ticket & RW_KERNPROC) != 0;
}

int lock_wait_time = 100;

void
rwlock_pause(rwl, wanted)
	struct rwlock *rwl;
	int wanted;
{
	if (lock_wait_time > 0) {
		int i;
		rwlock_unlock(&rwl);
		for(i = lock_wait_time; i > 0; i--) {
			if (!(wanted)) {
				break;
			}
		}
		rwlock_lock(&rwl);
	}
	if (!(wanted)) {
		break;
	}
}

void
rwlock_acquire(rwl, error, extflags, wanted)
	struct rwlock *rwl;
	int error, extflags, wanted;
{
	rwlock_pause(rwl, wanted);
	for (error = 0; wanted; ) {
		rwl->rwl_waitcount++;
		rwlock_unlock(&rwl);
		error = tsleep((void *)rwl, rwl->rwl_prio, rwl->rwl_timo);
		rwlock_lock(&rwl);
		rwl->rwl_waitcount--;
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
rwlock_lock(rwl)
    __volatile rwlock_t rwl;
{
	simple_lock(&rwl->rwl_lnterlock);
}

void
rwlock_unlock(rwl)
    __volatile rwlock_t rwl;
{
	simple_unlock(&rwl->rwl_lnterlock);
}
