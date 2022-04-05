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

#ifndef SYS_RWLOCK_H_
#define SYS_RWLOCK_H_

#include <sys/lock.h>

/* Reader Writers Lock */
struct rwlock {
	struct lock_object		rwl_lnterlock;		/* lock object */
    struct lock_holder		*rwl_lockholder;	/* lock holder */

    int 					rwl_readercount;	/* lock reader count */
    int						rwl_writercount;	/* lock writer count */
    int						rwl_waitcount;		/* # of processes sleeping for lock */

    u_int					rwl_flags;			/* see below */
    short					rwl_prio;			/* priority at which to sleep */
    char					*rwl_wmesg;			/* resource sleeping (for tsleep) */
    int						rwl_timo;			/* maximum sleep time (for tsleep) */
};

#define RW_KERNPROC  		LK_KERNPROC
#define RW_NOPROC   		LK_NOPROC

/* These are flags that are passed to the rwlockmgr routine. */
#define RW_TYPE_MASK	    0x0FFFFFFF
#define RW_READER			0x00000001
#define RW_WRITER			0x00000002
#define RW_UPGRADE	        0x00000003			/* read-to-write upgrade */
#define RW_DOWNGRADE	    0x00000004			/* write-to-read downgrade */

/* External lock flags. */
#define RW_EXTFLG_MASK	    LK_EXTFLG_MASK		/* mask of external flags */
#define RW_NOWAIT	        LK_NOWAIT			/* do not sleep to await lock */
#define RW_WAIT	        	0x00000020			/* sleep to await lock */

/* Internal lock flags. */
#define RW_WANT_UPGRADE		0x00000100			/* waiting for reader-to-writer upgrade */
#define RW_WANT_WRITE	    0x00000200			/* writer lock sought >= 1 waiter is a writer */
#define RW_HAVE_WRITE	    0x00000400			/* writer lock obtained and currently write locked */
#define RW_WANT_DOWNGRADE	0x00000800			/* waiting for writer-to-reader downgrade */
#define RW_WANT_READ	    0x00004000			/* reader lock sought */
#define RW_HAVE_READ	    0x00008000			/* reader lock obtained */

#define	RW_HAS_WAITERS		0x01UL				/* lock has waiters */
#define	RW_NODEBUG			0x10UL				/* LOCKDEBUG disabled */

/* Control flags. */
#define RW_INTERLOCK		LK_INTERLOCK		/* unlock passed simple lock after getting lk_interlock */
#define RW_RETRY			LK_RETRY			/* vn_lock: retry until locked */

void 			rwlock_init(struct rwlock *, int, char *, int, u_int);
int 			rwlockmgr(__volatile struct rwlock *, u_int, struct lock_object *, pid_t);
int 			rwlockstatus(struct rwlock *);

void			rwlock_lock(__volatile struct rwlock *);
void			rwlock_unlock(__volatile struct rwlock *);
int				rwlock_lock_try(__volatile struct rwlock *);
int				rwlock_read_held(struct rwlock *);
int				rwlock_write_held(struct rwlock *);
int				rwlock_lock_held(struct rwlock *);

#endif /* SYS_RWLOCK_H_ */

