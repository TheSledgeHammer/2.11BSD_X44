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

#ifndef _SYS_THREAD_H_
#define _SYS_THREAD_H_

#include <sys/proc.h>
#include <sys/mutex.h>

struct thread {
	TAILQ_ENTRY(thread) td_link;				/* Doubly-linked run queue. */
	LIST_ENTRY(thread)	td_list;				/* List of all threads */

    struct mtx			*td_mtx;				/* thread structure mutex */
	int	 				td_flag;				/* T_* flags. */
	char 				td_stat;				/* TS* thread status. */
	char 				td_lock;				/* Thread lock count. */

	short				td_uid;					/* user id, used to direct tty signals */
	pid_t 				td_tid;					/* unique thread id */
	pid_t 				td_ptid;				/* thread id of parent (proc) */

	void				*td_stack;				/* thread stack */
	size_t             	td_stacksize;			/* thread stack size */

	/* Substructures: */
	struct pcred 	 	*td_cred;				/* Thread owner's identity. */
#define	td_ucred		td_cred->pc_ucred

	struct proc 		*td_procp;				/* pointer to proc */
	struct user			*td_addr;				/* virtual address of u. area */

	LIST_ENTRY(thread)	td_sibling;				/* List of sibling threads. */
	LIST_ENTRY(thread)	td_hash;				/* Hash chain. */

	struct pgrp 	    *td_pgrp;       		/* Pointer to proc group. */

#define td_name			td_procp->p_comm
};

#define THREAD_STACK	USPACE

/* flag codes */
#define	TD_INMEM		0x00000004		/* Loaded into memory. */
#define TD_SULOCK		0x00000040		/* user settable lock in core */
#define	TD_SYSTEM		0x00000200		/* System proc: no sigs, stats or swapping. */
#define TD_TRACED		0x00000800		/* Debugged process being traced. */
#define	TD_NOSWAP		0x00008000		/* Another flag to prevent swap out. */
#define TD_INEXEC		0x00100000		/* Process is exec'ing and cannot be traced */
#define	TD_BOUND		0x80000000 		/* Bound to a CPU */

/* Kernel thread handling. */
#define	THREAD_IDLE		0x01			/* Do not run on creation */
#define	THREAD_MPSAFE	0x02			/* Do not acquire kernel_lock */
#define	THREAD_INTR		0x04			/* Software interrupt handler */
#define	THREAD_TS		0x08			/* Time-sharing priority range */
#define	THREAD_MUSTJOIN	0x10			/* Must join on exit */
#define THREAD_STEALABLE 0x12			/* thread able to be taken by another
										 * process aka thread is reparented
										 */

#ifdef _KERNEL
extern struct lock_holder 	thread_loholder;
#define THREAD_LOCK(td)		(mtx_lock(&(td)->td_mtx, &thread_loholder))
#define THREAD_UNLOCK(td) 	(mtx_unlock(&(td)->td_mtx, &thread_loholder))

#define	TID_MIN			31000
#define	TID_MAX			99000
#define NO_TID			NO_PID

#define	TIDHSZ							16
#define	TIDHASH(tid)					(&tidhashtbl[(tid) & tidhash & (TIDHSZ * ((tid) + tidhash) - 1)])
extern 	LIST_HEAD(tidhashhead, thread) 	*tidhashtbl;
extern u_long 	tidhash;

extern struct thread *curthread;				/* current running thread */
extern struct thread thread0;
extern int	nthread, maxthread;					/* Current and max number of threads. */
extern int ppnthreadmax;						/* max number of threads per proc (hits stack limit) */

LIST_HEAD(threadlist, thread);

struct thread *tdfind(struct proc *);			/* find thread by tidmask */
pid_t tidmask(struct proc *);					/* thread tidmask */
void tdqinit(struct thread *);
void threadinit(struct thread *);
void thread_add(struct proc *, struct thread *);
void thread_remove(struct proc *, struct thread *);
void thread_reparent(struct proc *, struct proc *, struct thread *);
void thread_steal(struct proc *, struct thread *);
struct thread *thread_alloc(struct proc *, size_t);
void thread_free(struct proc *, struct thread *);
void thread_hold(struct thread *); 			/* if thread state is running, thread blocks all siblings from running */
void thread_release(struct thread *);		/* if thread state is not running, thread unblocks all siblings from running */

/* kern_kthread.c */
int proc_create(struct proc **);
int	newthread(struct thread **, char *, size_t, bool_t);
int kthread_create(void (*)(void *), void *, struct thread **, char *, bool_t);

void kthread_exit(int);
void kthread_create_deferred(void (*)(void *), void *);
#endif 	/* KERNEL */
#endif /* _SYS_THREAD_H_ */
