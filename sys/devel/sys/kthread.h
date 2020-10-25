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
	struct vmspace  	*kt_vmspace;	/* Address space. */
	struct sigacts 		*kt_sigacts;	/* Signal actions, state (THREAD ONLY). */

#define	kt_ucred		kt_cred->pc_ucred
#define	kt_rlimit		kt_limit->pl_rlimit

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
#define KT_SSLEEP			1			/* sleeping/ awaiting an event */
#define KT_SWAIT			2			/* waiting */
#define KT_SRUN				3			/* running */
#define KT_SIDL				4			/* intermediate state in process creation */
#define	KT_SZOMB			5			/* intermediate state in process termination */
#define KT_SSTOP			6			/* process being traced */
#define KT_SREADY			7			/* ready */
#define KT_SSTART			8			/* start */

/* flag codes */
#define	KT_PPWAIT			0x00010		/* Parent is waiting for child to exec/exit. */
#define	KT_SYSTEM			0x00200		/* System proc: no sigs, stats or swapping. */
#define	KT_INMEM			0x00004		/* Loaded into memory. */
#define KT_INEXEC			0x100000	/* Process is exec'ing and cannot be traced */

#define	KT_NOSWAP			0x08000		/* Another flag to prevent swap out. */

#define	KT_BOUND			0x80000000 	/* Bound to a CPU */

/* Kernel thread handling. */
#define	KTHREAD_IDLE		0x01		/* Do not run on creation */
#define	KTHREAD_MPSAFE		0x02		/* Do not acquire kernel_lock */
#define	KTHREAD_INTR		0x04		/* Software interrupt handler */
#define	KTHREAD_TS			0x08		/* Time-sharing priority range */
#define	KTHREAD_MUSTJOIN	0x10		/* Must join on exit */

/* Kernel Threadpool Threads */
TAILQ_HEAD(kthread_head, kthreadpool_thread);
struct kthreadpool_thread {
	struct proc							*ktpt_proc;					/* proc */
	struct kthread						*ktpt_kthread;				/* kernel threads */
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

#define	TIDHSZ							16
#define	TIDHASH(tid)					(&tidhashtbl[(tid) & tid_hash & (TIDHSZ * ((tid) + tid_hash) - 1)])
extern 	LIST_HEAD(tidhashhead, proc) 	*tidhashtbl;
u_long 	tid_hash;

#define	TGRPHASH(tgid)					(&tgrphashtbl[(tgid) & tgrphash])
extern LIST_HEAD(tgrphashhead, pgrp) 	*tgrphashtbl;
extern u_long tgrphash;

struct kthread 							*kthreadNKTHREAD;		/* the kthread table itself */

struct kthread 							*allkthread;			/* List of active kthreads. */
struct kthread 							*freekthread;			/* List of free kthreads. */
struct kthread 							*zombkthread;			/* List of zombie kthreads. */

lock_t 									kthread_lkp; 			/* lock */
rwlock_t								kthread_rwl;			/* reader-writers lock */

extern struct kthread 					kthread0;
extern struct kthreadpool_thread 		ktpool_thread;
extern lock_t 							kthreadpool_lock;

struct kthread *ktfind (pid_t);				/* Find kthread by id. */
int				leavetgrp(kthread_t);

void			threadinit (void);
struct pgrp 	*tgfind (pid_t);			/* Find thread group by id. */

/* Kernel Thread ITPC */
extern void kthreadpool_itc_send(struct kthreadpool *, struct threadpool_itpc *);
extern void kthreadpool_itc_receive(struct kthreadpool *, struct threadpool_itpc *);

/* Kernel Thread */
void kthread_init(struct proc *, kthread_t);
int kthread_create(kthread_t kt);
int kthread_join(kthread_t kt);
int kthread_cancel(kthread_t kt);
int kthread_exit(kthread_t kt);
int kthread_detach(kthread_t kt);
int kthread_equal(kthread_t kt1, kthread_t kt2);
int kthread_kill(kthread_t kt);
int kthread_lock_init(lock_t, kthread_t);
int kthread_lockmgr(lock_t, u_int, kthread_t);
int kthread_rwlock_init(rwlock_t, kthread_t);
int kthread_rwlockmgr(rwlock_t, u_int, kthread_t);
int	kthread_rwlock_read_held(kthread_t, rwlock_t);
int	kthread_rwlock_write_held(kthread_t, rwlock_t);
int	kthread_rwlock_lock_held(kthread_t, rwlock_t);

#endif /* SYS_KTHREADS_H_ */
