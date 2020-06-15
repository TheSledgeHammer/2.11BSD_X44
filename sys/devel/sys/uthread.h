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
 /*  User Threads: Runs from user struct in Userspace */

#ifndef SYS_UTHREADS_H_
#define SYS_UTHREADS_H_

#include <sys/user.h>

/* user threads */
struct uthread {
	struct uthread		*ut_nxt;		/* linked list of allocated thread slots */
	struct uthread		**ut_prev;

	struct uthread 		*ut_forw;		/* Doubly-linked run/sleep queue. */
	struct uthread 		*ut_back;

	int	 				ut_flag;		/* T_* flags. */
	char 				ut_stat;		/* TS* thread status. */
	char 				ut_lock;		/* Thread lock count. */

	short 				ut_tid;			/* unique thread id */
	short 				ut_ptid;		/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*ut_cred;		/* Thread owner's identity. */
	struct filedesc 	*ut_fd;			/* Ptr to open files structure. */
	struct pstats 	 	*ut_stats;		/* Accounting/statistics (PROC ONLY). */
	struct sigacts 		*ut_sigacts;	/* Signal actions, state (PROC ONLY). */

	struct user 		*ut_userp;		/* Pointer to User Structure */
	struct kthread		*ut_kthreadp;	/* Pointer to Kernel Thread Structure */

	struct uthread    	*ut_hash;       /* hashed based on t_tid & p_pid for kill+exit+... */
	struct uthread    	*ut_tgrpnxt;	/* Pointer to next thread in thread group. */
    struct uthread      *ut_tptr;		/* pointer to process structure of parent */
    struct uthread 		*ut_ostptr;	 	/* Pointer to older sibling processes. */

	struct uthread 		*ut_ysptr;	 	/* Pointer to younger siblings. */
	struct uthread 		*ut_cptr;	 	/* Pointer to youngest living child. */

	struct tgrp 	    *ut_tgrp;       /* Pointer to thread group. */

	struct mutex        *ut_mutex;
	struct rwlock		*ut_rwlock;
    short               ut_locks;
    short               ut_simple_locks;
};
#define	ut_session		ut_tgrp->tg_session
#define	ut_tgid			ut_tgrp->tg_id

/* Locks */
mutex_t 				uthread_mtx; 		/* mutex lock */

/* User Threadpool Thread */
TAILQ_HEAD(uthread_head, uthreadpool_thread);
struct uthreadpool_thread {
	struct uthreads						*utpt_uthread;		/* user threads */
    char				                *utpt_uthread_savedname;
	struct user_threadpool				*utpt_pool;
	struct threadpool_job				*utpt_job;

    TAILQ_ENTRY(uthreadpool_thread) 	utpt_entry;			/* list uthread entries */
};

/* User Threadpool */
struct uthreadpool {
	mutex_t								utp_lock;
	struct uthreadpool_thread			utp_overseer;
    struct job_head					    utp_jobs;
    struct uthread_head		            utp_idle_threads;

    int 								utp_refcnt;			/* total thread count in pool */
    int 								utp_active;			/* active thread count */
    int									utp_inactive;		/* inactive thread count */

    /* Inter Threadpool Communication */
    struct threadpool_itpc				utp_itc;			/* threadpool ipc ptr */
    boolean_t							utp_issender;		/* is itc sender */
    boolean_t							utp_isreciever;		/* is itc reciever */
    int									utp_retcnt;			/* retry count in itc pool */

    boolean_t							utp_initcq;			/* check if in itc queue */
    //flags, refcnt, priority, lock
    //flags: send, (verify: success, fail), retry_attempts
};

extern struct uthread uthread0;

extern void uthreadpool_itc_send(struct uthreadpool *, struct threadpool_itpc *);
extern void uthreadpool_itc_receive(struct uthreadpool *, struct threadpool_itpc *);

/* User Thread */
int uthread_create(uthread_t ut);
int uthread_join(uthread_t ut);
int uthread_cancel(uthread_t ut);
int uthread_exit(uthread_t ut);
int uthread_detach(uthread_t ut);
int uthread_equal(uthread_t ut1, uthread_t ut2);
int uthread_kill(uthread_t ut);

/* User Thread Group */
extern struct uthread *utidhash[];		/* In param.c. */

struct uthread 	*utfind (tid_t);		/* Find user thread by id. */
int				leavetgrp(struct uthread *);
int				entertgrp(struct uthread *, tid_t, int);
void			fixjobc(struct uthread *, struct tgrp *, int);
int				inferior(struct uthread *);

/* User Thread Mutex */
int uthread_mutexmgr(mutex_t, u_int, uthread_t);
int uthread_mutex_init(mutex_t, uthread_t);
int uthread_mutex_lock(uthread_t, mutex_t);
int uthread_mutex_lock_try(uthread_t, mutex_t);
int uthread_mutex_timedlock(uthread_t, mutex_t);
int uthread_mutex_unlock(uthread_t, mutex_t);
int uthread_mutex_destroy(uthread_t, mutex_t);

/* User Thread rwlock */
int uthread_rwlock_init(rwlock_t, uthread_t);
int uthread_rwlockmgr(rwlock_t, u_int, uthread_t);

#endif /* SYS_UTHREADS_H_ */
