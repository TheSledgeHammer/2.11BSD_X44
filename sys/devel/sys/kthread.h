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
	LIST_ENTRY(kthread)	kt_list;				/* List of all kernel threads */

    struct mtx			*kt_mtx;				/* kthread structure mutex */
	int	 				kt_flag;				/* T_* flags. */
	char 				kt_stat;				/* TS* thread status. */
	char 				kt_lock;				/* Thread lock count. */

	short				kt_uid;					/* user id, used to direct tty signals */
	pid_t 				kt_tid;					/* unique thread id */
	pid_t 				kt_ptid;				/* thread id of parent */

	size_t             	kt_stack;

	/* Substructures: */
	struct pcred 	 	*kt_cred;				/* Thread owner's identity. */
#define	kt_ucred		kt_cred->pc_ucred

	struct proc 		*kt_procp;				/* pointer to proc */
	struct uthread		*kt_uthreado;			/* uthread overseer (original uthread)  */

	LIST_ENTRY(proc) 	kt_pglist;				/* List of kthreads in pgrp. */

	LIST_ENTRY(kthread)	kt_sibling;				/* List of sibling kthreads. */
	LIST_ENTRY(kthread)	kt_hash;				/* Hash chain. */

	struct pgrp 	    *kt_pgrp;       		/* Pointer to proc group. */
	struct kthread 		*kt_link;				/* linked list of running kthreads */

    u_short 			kt_acflag;	    		/* Accounting flags. */

#define kt_name			kt_procp->p_comm

	struct mpx 			*kt_mpx;

	struct kthread_runtime	*kt_runtime;
};

#define	kt_session		kt_pgrp->pg_session
#define	kt_tgid			kt_pgrp->pg_id

/* dispatch/stat codes */
#define KT_ONALL			0			/* on allkthread list */
#define KT_ONFREE			1			/* on freekthread list */
#define KT_ONZOMB			2			/* on zombkthread list */

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
extern struct kthreadlist 				freekthread;			/* List of free kthreads. */

struct lock								*kthread_lkp; 			/* lock */
struct rwlock							*kthread_rwl;			/* reader-writers lock */

extern struct kthread 					kthread0;

/* KThread Lock */
struct lock_holder 			kthread_loholder;
#define KTHREAD_LOCK(kt)	(mtx_lock(&(kt)->kt_mtx, &kthread_loholder))
#define KTHREAD_UNLOCK(kt) 	(mtx_unlock(&(kt)->kt_mtx, &kthread_loholder))

void			threadinit(void);
struct pgrp 	*tgfind(pid_t);
void			tgdelete(struct pgrp *);

void 			kthread_init(struct proc *, struct kthread *);
struct kthread *ktfind(pid_t);				/* Find kthread by id. */
int				leavektgrp(kthread_t);		/* leave thread group */

/* KThread ITPC */
extern void		itpc_add_kthreadpool(struct threadpool_itpc *, struct kthreadpool *);
extern void		itpc_remove_kthreadpool(struct threadpool_itpc *, struct kthreadpool *);
void 			itpc_check_kthreadpool(struct threadpool_itpc *, pid_t);
void 			itpc_verify_kthreadpool(struct threadpool_itpc *, pid_t);

#endif /* SYS_KTHREADS_H_ */
