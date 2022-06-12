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
	struct uthread 		*ut_forw;				/* Doubly-linked run/sleep queue. */
	struct uthread 		*ut_back;

	LIST_ENTRY(uthread)	ut_list;

    struct mtx			*ut_mtx;				/* uthread structure mutex */
	int	 				ut_flag;				/* T_* flags. */
	char 				ut_stat;				/* TS* thread status. */
	char 				ut_lock;				/* Thread lock count. */

	short				ut_uid;					/* user id, used to direct tty signals */
	pid_t 				ut_tid;					/* unique thread id */
	pid_t 				ut_ptid;				/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*ut_cred;				/* Thread owner's identity. */
	struct filedesc 	*ut_fd;					/* Ptr to open files structure. */
	struct pstats 	 	*ut_stats;				/* Accounting/statistics (THREAD ONLY). */
	struct sigacts 		*ut_sigacts;			/* Signal actions, state (THREAD ONLY). */

#define	ut_ucred		ut_cred->pc_ucred

	struct user 		*ut_userp;				/* pointer to user */
	struct kthread		*ut_kthreadp;			/* pointer to kernel threads */

	LIST_ENTRY(proc) 	ut_pglist;				/* List of uthreads in pgrp. */
	struct uthread      *ut_pptr;				/* pointer to process structure of parent */
	LIST_ENTRY(uthread)	ut_sibling;				/* List of sibling uthreads. */
	LIST_HEAD(, uthread)ut_children;			/* Pointer to list of children. */
	LIST_ENTRY(proc)	ut_hash;				/* Hash chain. */

#define	ut_startzero	ut_oppid

	 pid_t				ut_oppid;	    		/* Save parent pid during ptrace. XXX */

#define	ut_endzero		ut_startcopy
#define	ut_startcopy	ut_sigmask

	u_char				ut_pri;					/* thread priority, negative is high */
	char				ut_comm[MAXCOMLEN+1];	/* p: basename of last exec file */

	struct pgrp 	    *ut_pgrp;       		/* Pointer to proc group. */

#define ut_endcopy

	struct uthread 		*ut_link;				/* linked list of running uthreads */
    char				*ut_name;				/* (: name, optional */
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
	struct kthread						*utpt_kthread;
	struct uthread						*utpt_uthread;		/* user threads */
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
    struct threadpool_itpc				utp_itpc;			/* threadpool ipc ptr */
    bool_t								utp_issender;		/* is itc sender */
    bool_t								utp_isreciever;		/* is itc reciever */
    int									utp_retcnt;			/* retry count in itc pool */
    bool_t								utp_initcq;			/* check if in itc queue */
};

LIST_HEAD(uthreadlist, uthread);
extern struct uthreadlist				alluthread;			/* List of active uthreads. */
extern struct uthreadlist				zombuthread;		/* List of zombie uthreads. */
//extern struct uthreadlist 				freeuthread;		/* List of free uthreads. */

lock_t									uthread_lkp;		/* lock */
rwlock_t								uthread_rwl;		/* reader-writers lock */

extern struct uthread 					uthread0;
extern struct uthreadpool_thread 		utpool_thread;
extern struct lock 						*uthreadpool_lock;

/* UThread Lock */
struct lock_holder 			uthread_loholder;
#define UTHREAD_LOCK(ut)	(mtx_lock(&(ut)->ut_mtx, &uthread_loholder))
#define UTHREAD_UNLOCK(ut) 	(mtx_unlock(&(ut)->ut_mtx, &uthread_loholder))

/* UThread */
void 			uthread_init(struct kthread *, struct uthread *);
int	 			uthread_create(void (*)(void *), void *, struct kthread **, const char *);
int 			uthread_exit(int);
void			uthread_create_deferred(void (*)(void *), void *);
void			uthread_run_deferred_queue(void);

struct uthread *utfind (pid_t);			/* Find uthread by id. */
int				leaveutgrp(uthread_t);	/* leave thread group */

/* UThread ITPC */
extern void 	uthreadpool_itpc_send(struct threadpool_itpc *, struct uthreadpool *);
extern void 	uthreadpool_itpc_receive(struct threadpool_itpc *, struct uthreadpool *);
extern void		itpc_add_uthreadpool(struct threadpool_itpc *, struct uthreadpool *);
extern void		itpc_remove_uthreadpool(struct threadpool_itpc *, struct uthreadpool *);
void 			itpc_check_uthreadpool(struct threadpool_itpc *, pid_t);
void 			itpc_verify_uthreadpool(struct threadpool_itpc *, pid_t);

#endif /* SYS_UTHREADS_H_ */
