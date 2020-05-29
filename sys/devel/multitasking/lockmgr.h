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

/* To become lock.h: after rwlock & mutex are fully implemented */
#ifndef SYS_LOCKMGR_H_
#define SYS_LOCKMGR_H_

/*
 * The general lock structure.  Provides for multiple shared locks,
 * upgrading from shared to exclusive, and sleeping until the lock
 * can be gained. The simple locks are defined in <machine/param.h>.
 */

struct lockmgr {
    struct	simplelock 	lk_interlock; 		/* lock on remaining fields */

    int					lk_waitcount;		/* # of processes sleeping for lock */
    short				lk_prio;			/* priority at which to sleep */
    char				*lk_wmesg;			/* resource sleeping (for tsleep) */
    int					lk_timo;			/* maximum sleep time (for tsleep) */
};

/*
 * External lock flags.
 *
 * The first three flags may be set in lock_init to set their mode permanently,
 * or passed in as arguments to the lock manager. The LK_REENABLE flag may be
 * set only at the release of a lock obtained by drain.
 */
#define LK_EXTFLG_MASK	0x00000070			/* mask of external flags */
#define LK_NOWAIT		0x00000010			/* do not sleep to await lock */
#define LK_SLEEPFAIL	0x00000020			/* sleep, then return failure */
#define LK_CANRECURSE	0x00000040			/* allow recursive exclusive lock */
#define LK_REENABLE		0x00000080			/* lock is be reenabled after drain */

/*
 * Control flags
 *
 * Non-persistent external flags.
 */
#define LK_INTERLOCK	0x00010000			/* unlock passed simple lock after getting lk_interlock */
#define LK_RETRY		0x00020000			/* vn_lock: retry until locked */

/*
 * Indicator that no process holds exclusive lock
 */
#define LK_KERNPROC 	((pid_t) -2)
#define LK_NOPROC 		((pid_t) -1)
#define LK_THREAD  		((tid_t) -2)
#define LK_NOTHREAD    	((tid_t) -1)
struct proc;

extern void pause(struct lock *, int);
extern void acquire(struct lock *, int, int, int);
extern void count(struct proc *, short);

#ifdef DEBUG
void 	_simple_unlock (__volatile struct simplelock *alp, const char *, int);
#define simple_unlock(alp) _simple_unlock(alp, __FILE__, __LINE__)
int 	_simple_lock_try (__volatile struct simplelock *alp, const char *, int);
#define simple_lock_try(alp) _simple_lock_try(alp, __FILE__, __LINE__)
void 	_simple_lock (__volatile struct simplelock *alp, const char *, int);
#define simple_lock(alp) _simple_lock(alp, __FILE__, __LINE__)
void 	simple_lock_init (struct simplelock *alp);
#else /* !DEBUG */
#if NCPUS == 1 /* no multiprocessor locking is necessary */
#define	simple_lock_init(alp)
#define	simple_lock(alp)
#define	simple_lock_try(alp)	(1)	/* always succeeds */
#define	simple_unlock(alp)
#endif /* NCPUS == 1 */
#endif /* !DEBUG */
#endif /* SYS_LOCKMGR_H_ */
