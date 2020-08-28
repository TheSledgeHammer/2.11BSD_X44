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

/* Number of Threads per Process? */

/* kernel threads */
typedef struct kthread 	*kthread_t;
struct kthread {
	struct kthread		*kt_nxt;		/* linked list of allocated thread slots */
	struct kthread		**kt_prev;

	struct kthread 		*kt_forw;		/* Doubly-linked run/sleep queue. */
	struct kthread 		*kt_back;

	int	 				kt_flag;		/* T_* flags. */
	char 				kt_stat;		/* TS* thread status. */
	char 				kt_lock;		/* Thread lock count. */

	short				kt_uid;			/* user id, used to direct tty signals */
	pid_t 				kt_tid;			/* unique thread id */
	pid_t 				kt_ptid;		/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*kt_cred;		/* Thread owner's identity. */
	struct filedesc 	*kt_fd;			/* Ptr to open files structure. */
	struct pstats 	 	*kt_stats;		/* Accounting/statistics (THREAD ONLY). */
	struct plimit 	 	*kt_limit;		/* Process limits. */
	struct sigacts 		*kt_sig;		/* Signal actions, state (THREAD ONLY). */

#define	kt_ucred		kt_cred->pc_ucred

	struct proc 		*kt_procp;		/* pointer to proc */
	struct uthread		*kt_uthreado;	/* uthread overseer (original uthread)  */

	LIST_ENTRY(proc)	kt_hash;		/* hashed based on t_tid & p_pid for kill+exit+... */
	struct kthread    	*kt_pgrpnxt;	/* Pointer to next thread in thread group. */
    struct kthread      *kt_pptr;		/* pointer to process structure of parent */
    struct kthread 		*kt_ostptr;	 	/* Pointer to older sibling processes. */

#define	kt_startzero	kt_ysptr

	struct kthread 		*kt_ysptr;	 	/* Pointer to younger siblings. */
	struct kthread 		*kt_cptr;	 	/* Pointer to youngest living child. */

    u_int				kt_estcpu;	 	/* Time averaged value of p_cpticks. */

    caddr_t				kt_wchan;		/* event process is awaiting */
    caddr_t				kt_wmesg;	 	/* Reason for sleep. */

	struct itimerval   	kt_realtimer;	/* Alarm timer. */
	struct timeval     	kt_rtime;	    /* Real time. */

	struct vnode 	    *kt_textvp;		/* Vnode of executable. */

#define	kt_endzero		kt_startcopy
#define	kt_startcopy	kt_sigmask

    sigset_t 			kt_sigmask;		/* Current signal mask. */
    sigset_t 			kt_sigignore;	/* Signals being ignored */
    sigset_t 			kt_sigcatch;	/* Signals being caught by user */

	u_char				kt_pri;			/* thread  priority, negative is high */
	u_char				kt_cpu;			/* cpu usage for scheduling */
	char				kt_nice;		/* nice for cpu usage */
	char				kt_slptime;		/* Time since last blocked. secs sleeping */

	struct pgrp 	    *kt_pgrp;       /* Pointer to proc group. */
	struct sysentvec	*kt_sysent;		/* System call dispatch information. */

#define kt_endcopy

	struct kthread 		*kt_link;		/* linked list of running kthreads */

    u_short 			kt_acflag;	    /* Accounting flags. */

    short               kt_locks;
    short               kt_simple_locks;

	char				*kt_name;		/* (: name, optional */
};
#define	kt_session		kt_pgrp->pg_session
#define	kt_tgid			kt_pgrp->pg_id

/* stat codes */
#define KTSSLEEP	1		/* sleeping/ awaiting an event */
#define KTSWAIT		2		/* waiting */
#define KTSRUN		3		/* running */
#define KTSIDL		4		/* intermediate state in process creation */
#define	KTSZOMB		5		/* intermediate state in process termination */
#define KTSSTOP		6		/* process being traced */
#define KTSREADY	7		/* ready */
#define KTSSTART	8		/* start */

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
	lock_t								ktp_lock;
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
};

lock_t 									kthread_lkp; 	/* lock */
rwlock_t								kthread_rwl;	/* reader-writers lock */

extern struct kthread 					kthread0;
extern struct kthreadpool_thread 		ktpool_thread;
extern lock_t 							kthreadpool_lock;

struct kthread *kthreadNKTHREAD;		/* the kthread table itself */

struct kthread *allkthread;				/* List of active kthreads. */
struct kthread *freekthread;			/* List of free kthreads. */
struct kthread *zombkthread;			/* List of zombie kthreads. */

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

/* Kernel Thread Lock */
int kthread_lock_init(lock_t, kthread_t);
int kthread_lockmgr(lock_t, u_int, kthread_t);

/* Kernel Thread rwlock */
int kthread_rwlock_init(rwlock_t, kthread_t);
int kthread_rwlockmgr(rwlock_t, u_int, kthread_t);
int	kthread_rwlock_read_held(kthread_t, rwlock_t);
int	kthread_rwlock_write_held(kthread_t, rwlock_t);
int	kthread_rwlock_lock_held(kthread_t, rwlock_t);

#endif /* SYS_KTHREADS_H_ */
