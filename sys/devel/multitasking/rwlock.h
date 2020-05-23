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
#include <mutex.h>
#include <tcb.h>

typedef enum rwl_t {
	RW_READER = 0,
	RW_WRITER = 1
} rwl_t;

/* Reader Writers Lock */
struct rwlock {
    const char              *rwl_ident;
    unsigned int   			rwl_lock;

    struct kthread          *rwl_ktlockholder; 	/* Kernel Thread lock holder */
    struct uthread          *rwl_utlockholder;	/* User Thread lock holder */

    struct mutex			*rwl_mutex;			/* mutex lock */
    struct simplelock       *rwl_interlock;     /* lock on remaining fields */
    int					    rwl_sharecount;		/* # of accepted shared locks */
    int					    rwl_waitcount;		/* # of processes sleeping for lock */
    short				    rw_exclusivecount;	/* # of recursive exclusive locks */

    unsigned int		    rwl_flags;			/* see below */
    short				    rwl_prio;			/* priority at which to sleep */
    char				    *rwl_wmesg;			/* resource sleeping (for tsleep) */
    int					    rwl_timo;			/* maximum sleep time (for tsleep) */

    tid_t                   rwl_lockholder;
};

#define RW_THREAD  			((tid_t) -2)
#define RW_NOTHREAD    		((tid_t) -1)

#define RW_EXTFLG_MASK	    0x00000070	/* mask of external flags */

#define	RW_HAS_WAITERS		0x01UL		/* lock has waiters */
#define	RW_WRITE_WANTED		0x02UL		/* >= 1 waiter is a writer */
#define	RW_WRITE_LOCKED		0x04UL		/* lock is currently write locked */
#define	RW_NODEBUG			0x10UL		/* LOCKDEBUG disabled */

#define RW_INTERLOCK		0x00010000	/* unlock passed simple lock after getting lk_interlock */
#define RW_RETRY			0x00020000	/* vn_lock: retry until locked */

void 	rwlock_init(rwlock_t, int, char *, int, u_int);
void 	rwlock_mutex_init(rwlock_t, mutex_t);
int 	rwlock_destroy(rwlock_t);

int		rwlock_tryenter(rwlock_t, const rwl_t);
int		rwlock_tryupgrade(rwlock_t);
void	rwlock_downgrade(rwlock_t);

int		rwlock_read_held(rwlock_t);
int		rwlock_write_held(rwlock_t);
int		rwlock_lock_held(rwlock_t);

void	rwlock_enter(rwlock_t, const rwl_t);
void	rwlock_exit(rwlock_t);

#endif /* SYS_RWLOCK_H_ */
