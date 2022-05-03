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
#include <sys/mutex.h>

/* kernel threads */
typedef struct kthread 	*kthread_t;
struct kthread {
	struct kthread 		*kt_forw;				/* Doubly-linked run/sleep queue. */
	struct kthread 		*kt_back;

	LIST_ENTRY(kthread)	kt_list;				/* List of all kernel threads */

    struct mtx			*kt_mtx;				/* kthread structure mutex */
	int	 				kt_flag;				/* T_* flags. */
	char 				kt_stat;				/* TS* thread status. */
	char 				kt_lock;				/* Thread lock count. */

	short				kt_uid;					/* user id, used to direct tty signals */
	pid_t 				kt_tid;					/* unique thread id */
	pid_t 				kt_ptid;				/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*kt_cred;				/* Thread owner's identity. */
	struct filedesc 	*kt_fd;					/* Ptr to open files structure. */
	struct pstats 	 	*kt_stats;				/* Accounting/statistics (THREAD ONLY). */
	struct plimit 	 	*kt_limit;				/* Process limits. */
	struct vmspace  	*kt_vmspace;			/* Address space. */
	struct sigacts 		*kt_sigacts;			/* Signal actions, state (THREAD ONLY). */

#define	kt_ucred		kt_cred->pc_ucred
#define	kt_rlimit		kt_limit->pl_rlimit

	struct proc 		*kt_procp;				/* pointer to proc */
	struct uthread		*kt_uthreado;			/* uthread overseer (original uthread)  */

	LIST_ENTRY(proc) 	kt_pglist;				/* List of kthreads in pgrp. */
	struct kthread      *kt_pptr;				/* pointer to process structure of parent */
	LIST_ENTRY(kthread)	kt_sibling;				/* List of sibling kthreads. */
	LIST_HEAD(, kthread)kt_children;			/* Pointer to list of children. */
	LIST_ENTRY(proc)	kt_hash;				/* Hash chain. */

#define	kt_startzero	kt_oppid

	 pid_t				kt_oppid;	    		/* Save parent pid during ptrace. XXX */

    u_int				kt_estcpu;	 			/* Time averaged value of p_cpticks. */

    caddr_t				kt_wchan;				/* event process is awaiting */
    caddr_t				kt_wmesg;	 			/* Reason for sleep. */

	struct itimerval   	kt_realtimer;			/* Alarm timer. */
	struct timeval     	kt_rtime;	    		/* Real time. */

	struct vnode 	    *kt_textvp;				/* Vnode of executable. */

#define	kt_endzero		kt_startcopy
#define	kt_startcopy	kt_sigmask

    sigset_t 			kt_sigmask;				/* Current signal mask. */
    sigset_t 			kt_sigignore;			/* Signals being ignored */
    sigset_t 			kt_sigcatch;			/* Signals being caught by user */

	u_char				kt_pri;					/* thread  priority, negative is high */
	u_char				kt_cpu;					/* cpu usage for scheduling */
	char				kt_nice;				/* nice for cpu usage */
	char				kt_slptime;				/* Time since last blocked. secs sleeping */
	char				kt_comm[MAXCOMLEN+1];	/* p: basename of last exec file */

	struct pgrp 	    *kt_pgrp;       		/* Pointer to proc group. */
	struct sysentvec	*kt_sysent;				/* System call dispatch information. */

#define kt_endcopy

	struct kthread 		*kt_link;				/* linked list of running kthreads */

    u_short 			kt_acflag;	    		/* Accounting flags. */

	char				*kt_name;				/* (: name, optional */

	vm_offset_t			kt_kstack;				/* (a) Kernel VA of kstack. */
	int					kt_kstack_pages; 		/* (a) Size of the kstack. */
	vm_object_t			kt_kstack_obj;

    //struct sadata_vp 	*kt_savp; 				/* SA "virtual processor" */
};
#define	kt_session		kt_pgrp->pg_session
#define	kt_tgid			kt_pgrp->pg_id

/* stat codes */
#define KT_SSLEEP			1				/* sleeping/ awaiting an event */
#define KT_SWAIT			2				/* waiting */
#define KT_SRUN				3				/* running */
#define KT_SIDL				4				/* intermediate state in process creation */
#define	KT_SZOMB			5				/* intermediate state in process termination */
#define KT_SSTOP			6				/* process being traced */
#define KT_SREADY			7				/* ready */
#define KT_SSTART			8				/* start */

/* flag codes */
#define	KT_INMEM			0x00000004		/* Loaded into memory. */
#define	KT_PPWAIT			0x00000010		/* Parent is waiting for child to exec/exit. */
#define KT_SULOCK			0x00000040		/* user settable lock in core */

#define	KT_SYSTEM			0x00000200		/* System proc: no sigs, stats or swapping. */
#define KT_TRACED			0x00000800		/* Debugged process being traced. */
#define	KT_NOCLDWAIT		0x00002000		/* No zombies if child dies */
#define	KT_NOSWAP			0x00008000		/* Another flag to prevent swap out. */
#define KT_INEXEC			0x00100000		/* Process is exec'ing and cannot be traced */

#define	KT_BOUND			0x80000000 		/* Bound to a CPU */

/* Kernel thread handling. */
#define	KTHREAD_IDLE		0x01			/* Do not run on creation */
#define	KTHREAD_MPSAFE		0x02			/* Do not acquire kernel_lock */
#define	KTHREAD_INTR		0x04			/* Software interrupt handler */
#define	KTHREAD_TS			0x08			/* Time-sharing priority range */
#define	KTHREAD_MUSTJOIN	0x10			/* Must join on exit */

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

    struct kthread_head		            ktp_active_threads;

    int 								ktp_refcnt;			/* total thread count in pool */
    int 								ktp_active;			/* active thread count */
    int									ktp_inactive;		/* inactive thread count */

#define	KTHREADPOOL_DYING				0x01
    int									ktp_flags;
    u_char								ktp_pri;			/* priority */

    /* Inter Threadpool Communication */
    struct threadpool_itpc				ktp_itpc;			/* threadpool ipc ptr */
    bool_t								ktp_issender;		/* is itpc sender */
    bool_t								ktp_isreciever;		/* is itpc reciever */
    int									ktp_retcnt;			/* retry count in itpc pool */
    bool_t								ktp_initcq;			/* check if in itpc queue */
};

#define	TIDHSZ							16
#define	TIDHASH(tid)					(&tidhashtbl[(tid) & tid_hash & (TIDHSZ * ((tid) + tid_hash) - 1)])
extern 	LIST_HEAD(tidhashhead, proc) 	*tidhashtbl;
u_long 	tid_hash;

#define	TGRPHASH(tgid)					(&tgrphashtbl[(tgid) & tgrphash])
extern LIST_HEAD(tgrphashhead, pgrp) 	*tgrphashtbl;
extern u_long tgrphash;

LIST_HEAD(kthreadlist, kthread);
extern struct kthreadlist				allkthread;				/* List of active kthreads. */
extern struct kthreadlist				zombkthread;			/* List of zombie kthreads. */
//extern struct kthreadlist 				freekthread;			/* List of free kthreads. */

lock_t 									kthread_lkp; 			/* lock */
rwlock_t								kthread_rwl;			/* reader-writers lock */

extern struct kthread 					kthread0;
extern struct kthreadpool_thread 		ktpool_thread;
extern lock_t 							kthreadpool_lock;

void			threadinit (void);
struct pgrp 	*tgfind (pid_t);
void			tgdelete (struct pgrp *);

/* KThread */
void 			kthread_init(struct proc *, struct kthread *);
int	 			kthread_create(void (*)(void *), void *, struct proc **, const char *);
int 			kthread_exit(int);
void			kthread_create_deferred(void (*)(void *), void *);
void			kthread_run_deferred_queue(void);

struct kthread *ktfind (pid_t);				/* Find kthread by id. */
int				leavektgrp (kthread_t);		/* leave thread group */

/* KThread ITPC */
extern void 	kthreadpool_itpc_send (struct threadpool_itpc *, struct kthreadpool *);
extern void 	kthreadpool_itpc_receive (struct threadpool_itpc *, struct kthreadpool *);
extern void		itpc_add_kthreadpool (struct threadpool_itpc *, struct kthreadpool *);
extern void		itpc_remove_kthreadpool (struct threadpool_itpc *, struct kthreadpool *);
void 			itpc_check_kthreadpool(struct threadpool_itpc *, pid_t);
void 			itpc_verify_kthreadpool(struct threadpool_itpc *, pid_t);

/* KThread Lock */
struct lock_holder 			kthread_loholder;
#define KTHREAD_LOCK(kt)	(mtx_lock(&(kt)->kt_mtx, &kthread_loholder))
#define KTHREAD_UNLOCK(kt) 	(mtx_unlock(&(kt)->kt_mtx, &kthread_loholder))
#endif /* SYS_KTHREADS_H_ */
