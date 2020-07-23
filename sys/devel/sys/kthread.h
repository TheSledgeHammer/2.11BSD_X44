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

#ifndef SYS_KTHREADS_H_
#define SYS_KTHREADS_H_

#include <sys/proc.h>
#include "sys/tcb.h"

/* Number of Threads per Process? */

/* kernel threads */
struct kthread {
	struct kthread		*kt_nxt;		/* linked list of allocated thread slots */
	struct kthread		**kt_prev;

	struct kthread 		*kt_forw;		/* Doubly-linked run/sleep queue. */
	struct kthread 		*kt_back;

	int	 				kt_flag;		/* T_* flags. */
	char 				kt_stat;		/* TS* thread status. */
	char 				kt_lock;		/* Thread lock count. */

	short 				kt_tid;			/* unique thread id */
	short 				kt_ptid;		/* thread id of parent */

	u_char				kt_pri;			/* thread  priority, negative is high */

	/* Substructures: */
	struct pcred 	 	*kt_cred;		/* Thread owner's identity. */
	struct filedesc 	*kt_fd;			/* Ptr to open files structure. */
	struct pstats 	 	*kt_stats;		/* Accounting/statistics (THREAD ONLY). */
	struct sigacts 		*kt_sig;		/* Signal actions, state (THREAD ONLY). */

#define	t_ucred			t_cred->pc_ucred

	struct proc 		*kt_procp;		/* Pointer to Proc */
	struct uthread		*kt_uthreadp;	/* Pointer User Threads */

	struct kthread    	*kt_hash;       /* hashed based on t_tid & p_pid for kill+exit+... */
	struct kthread    	*kt_tgrpnxt;	/* Pointer to next thread in thread group. */
    struct kthread      *kt_tptr;		/* pointer to process structure of parent */
    struct kthread 		*kt_ostptr;	 	/* Pointer to older sibling processes. */

	struct kthread 		*kt_ysptr;	 	/* Pointer to younger siblings. */
	struct kthread 		*kt_cptr;	 	/* Pointer to youngest living child. */

	struct tgrp 	    *kt_tgrp;       /* Pointer to thread group. */
	struct sysentvec	*kt_sysent;		/* System call dispatch information. */

	struct kthread 		*kt_link;		/* linked list of running threads */

	struct mutex        *kt_mutex;
	struct rwlock		*kt_rwlock;
    short               kt_locks;
    short               kt_simple_locks;
};
#define	kt_session		kt_tgrp->tg_session
#define	kt_tgid			kt_tgrp->tg_id

mutex_t 				kthread_mtx; 	/* mutex lock */
rwlock_t				kthread_rwl;	/* reader-writers lock */

/* Kernel Threadpool Threads */
TAILQ_HEAD(kthread_head, kthreadpool_thread);
struct kthreadpool_thread {
	struct proc							*ktpt_proc;
	struct kthreads						*ktpt_kthread;				/* kernel threads */
    char				                *ktpt_kthread_savedname;
	struct kthreadpool					*ktpt_pool;
	struct threadpool_job				*ktpt_job;

	TAILQ_ENTRY(kthreadpool_thread) 	ktpt_entry;					/* list kthread entries */
};

/* Kernel Threadpool */
struct kthreadpool {
	mutex_t								ktp_lock;
	struct kthreadpool_thread			ktp_overseer;
    struct job_head					    ktp_jobs;
    struct kthread_head		            ktp_idle_threads;

    int 								ktp_refcnt;			/* total thread count in pool */
    int 								ktp_active;			/* active thread count */
    int									ktp_inactive;		/* inactive thread count */

#define	KTHREADPOOL_DYING				0x01
    int									ktp_flags;
    struct cpu_info						*ktp_cpu;
    u_char								ktp_pri;			/* priority */

    /* Inter Threadpool Communication */
    struct threadpool_itpc				ktp_itc;			/* threadpool ipc ptr */
    boolean_t							ktp_issender;		/* is itc sender */
    boolean_t							ktp_isreciever;		/* is itc reciever */
    int									ktp_retcnt;			/* retry count in itc pool */
    boolean_t							ktp_initcq;			/* check if in itc queue */

    //flags, refcnt, priority, lock, verify
    //flags: send, (verify: success, fail), retry_attempts
};

extern struct kthread 					kthread0;
extern struct kthreadpool_thread 		ktpool_thread;
extern mutex_t 							kthreadpool_lock;

/* Kernel Thread ITPC */
extern void kthreadpool_itc_send(struct kthreadpool *, struct threadpool_itpc *);
extern void kthreadpool_itc_receive(struct kthreadpool *, struct threadpool_itpc *);

/* Kernel Thread */
int kthread_create(kthread_t kt);
int kthread_join(kthread_t kt);
int kthread_cancel(kthread_t kt);
int kthread_exit(kthread_t kt);
int kthread_detach(kthread_t kt);
int kthread_equal(kthread_t kt1, kthread_t kt2);
int kthread_kill(kthread_t kt);

/* Kernel Thread Mutex */
int kthread_mutexmgr(mutex_t, u_int, kthread_t);
int kthread_mutex_init(mutex_t, kthread_t);
int kthread_mutex_lock(kthread_t, mutex_t);
int kthread_mutex_lock_try(kthread_t, mutex_t);
int kthread_mutex_timedlock(kthread_t, mutex_t);
int kthread_mutex_unlock(kthread_t, mutex_t);
int kthread_mutex_destroy(kthread_t, mutex_t);

/* Kernel Thread rwlock */
int kthread_rwlock_init(rwlock_t, kthread_t);
int kthread_rwlockmgr(rwlock_t, u_int, kthread_t);

#endif /* SYS_KTHREADS_H_ */
