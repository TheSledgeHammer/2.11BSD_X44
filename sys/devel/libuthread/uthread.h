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

#include <sys/kthread.h>
#include <sys/user.h>

/* user threads */
typedef struct uthread 	*uthread_t;
struct uthread {
	struct uthread		*ut_nxt;		/* linked list of allocated thread slots */
	struct uthread		**ut_prev;

	struct uthread 		*ut_forw;		/* Doubly-linked run/sleep queue. */
	struct uthread 		*ut_back;

	int	 				ut_flag;		/* T_* flags. */
	char 				ut_stat;		/* TS* thread status. */
	char 				ut_lock;		/* Thread lock count. */

	short				ut_uid;			/* user id, used to direct tty signals */
	pid_t 				ut_tid;			/* unique thread id */
	pid_t 				ut_ptid;		/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*ut_cred;		/* Thread owner's identity. */
	struct filedesc 	*ut_fd;			/* Ptr to open files structure. */
	struct pstats 	 	*ut_stats;		/* Accounting/statistics (THREAD ONLY). */
	struct sigacts 		*ut_sigacts;	/* Signal actions, state (THREAD ONLY). */

#define	ut_ucred		ut_cred->pc_ucred

	struct user 		*ut_userp;		/* pointer to user */
	struct kthread		*ut_kthreadp;	/* pointer to kernel threads */

	LIST_ENTRY(proc)	ut_hash;		/* hashed based on t_tid & p_pid for kill+exit+... */
	struct uthread    	*ut_pgrpnxt;	/* Pointer to next thread in thread group. */
    struct uthread      *ut_pptr;		/* pointer to process structure of parent */
    struct uthread 		*ut_ostptr;	 	/* Pointer to older sibling processes. */

#define	ut_startzero	ut_ysptr

	struct uthread 		*ut_ysptr;	 	/* Pointer to younger siblings. */
	struct uthread 		*ut_cptr;	 	/* Pointer to youngest living child. */

#define	ut_endzero		ut_startcopy
#define	ut_startcopy	ut_sigmask

	u_char				ut_pri;			/* thread priority, negative is high */

	struct pgrp 	    *ut_pgrp;       /* Pointer to proc group. */

#define ut_endcopy

	struct uthread 		*ut_link;		/* linked list of running uthreads */

    short               ut_locks;
    short               ut_simple_locks;

    char				*ut_name;		/* (: name, optional */
};
#define	ut_session		ut_pgrp->pg_session
#define	ut_tgid			ut_pgrp->pg_id

/* stat codes */
#define UT_SSLEEP			1			/* sleeping/ awaiting an event */
#define UT_SWAIT			2			/* waiting */
#define UT_SRUN				3			/* running */
#define UT_SIDL				4			/* intermediate state in process creation */
#define	UT_SZOMB			5			/* intermediate state in process termination */
#define UT_SSTOP			6			/* process being traced */
#define UT_SREADY			7			/* ready */
#define UT_SSTART			8			/* start */

/* flag codes */
#define	UT_PPWAIT			0x00010		/* Parent is waiting for child to exec/exit. */
#define	UT_SYSTEM			0x00200		/* System proc: no sigs, stats or swapping. */
#define	UT_INMEM			0x00004		/* Loaded into memory. */
#define UT_INEXEC			0x100000	/* Process is exec'ing and cannot be traced */

#define	UT_BOUND			0x80000000 	/* Bound to a CPU */

/* Kernel thread handling. */
#define	UTHREAD_IDLE		0x01		/* Do not run on creation */
#define	UTHREAD_MPSAFE		0x02		/* Do not acquire kernel_lock */
#define	UTHREAD_INTR		0x04		/* Software interrupt handler */
#define	UTHREAD_TS			0x08		/* Time-sharing priority range */
#define	UTHREAD_MUSTJOIN	0x10		/* Must join on exit */

/* User Threadpool Thread */
TAILQ_HEAD(uthread_head, uthreadpool_thread);
struct uthreadpool_thread {
	struct kthreads						*utpt_kthread;
	struct uthreads						*utpt_uthread;		/* user threads */
    char				                *utpt_uthread_savedname;
	struct uthreadpool					*utpt_pool;
	struct threadpool_job				*utpt_job;

    TAILQ_ENTRY(uthreadpool_thread) 	utpt_entry;			/* list uthread entries */
};

/* User Threadpool */
struct uthreadpool {
	lock_t								utp_lock;
	struct uthreadpool_thread			utp_overseer;
    struct job_head					    utp_jobs;
    struct uthread_head		            utp_idle_threads;

    int 								utp_refcnt;			/* total thread count in pool */
    int 								utp_active;			/* active thread count */
    int									utp_inactive;		/* inactive thread count */

#define	UTHREADPOOL_DYING				0x01
    int									utp_flags;
    struct cpu_info						*utp_cpu;
    u_char								utp_pri;			/* priority */

    /* Inter Threadpool Communication */
    struct threadpool_itpc				utp_itc;			/* threadpool ipc ptr */
    boolean_t							utp_issender;		/* is itc sender */
    boolean_t							utp_isreciever;		/* is itc reciever */
    int									utp_retcnt;			/* retry count in itc pool */
    boolean_t							utp_initcq;			/* check if in itc queue */
};


struct uthread 							*uthreadNUTHREAD;	/* the uthread table itself */

struct uthread 							*alluthread;		/* List of active uthreads. */
struct uthread 							*freeuthread;		/* List of free uthreads. */
struct uthread 							*zombuthread;		/* List of zombie uthreads. */

lock_t									uthread_lkp;		/* lock */
rwlock_t								uthread_rwl;		/* reader-writers lock */

extern struct uthread 					uthread0;
extern struct uthreadpool_thread 		utpool_thread;
extern lock_t 							uthreadpool_lock;

/* UThread */
void uthread_init(kthread_t, uthread_t);
int uthread_create(uthread_t ut);
int uthread_join(uthread_t ut);
int uthread_cancel(uthread_t ut);
int uthread_exit(uthread_t ut);
int uthread_detach(uthread_t ut);
int uthread_equal(uthread_t ut1, uthread_t ut2);
int uthread_kill(uthread_t ut);

struct uthread *utfind (pid_t);
int				leaveutgrp(uthread_t);

/* UThread ITPC */
extern void uthreadpool_itc_send(struct threadpool_itpc *, struct uthreadpool *);
extern void uthreadpool_itc_receive(struct threadpool_itpc *, struct uthreadpool *);
extern void	itpc_add_uthreadpool(struct threadpool_itpc *, struct uthreadpool *);
extern void	itpc_remove_uthreadpool(struct threadpool_itpc *, struct uthreadpool *);

/* UThread Lock */
int 		uthread_lock_init(lock_t, uthread_t);
int 		uthread_lockmgr(lock_t, u_int, uthread_t);
int 		uthread_rwlock_init(rwlock_t, uthread_t);
int 		uthread_rwlockmgr(rwlock_t, u_int, uthread_t);

#endif /* SYS_UTHREADS_H_ */
