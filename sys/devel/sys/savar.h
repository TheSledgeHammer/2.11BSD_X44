/*	$NetBSD: savar.h,v 1.15 2004/03/14 01:08:47 cl Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Internal data usd by the scheduler activation implementation
 */

#ifndef _SYS_SAVAR_H_
#define _SYS_SAVAR_H_

#include <sys/lock.h>
#include <sys/tree.h>
#include <sys/queue.h>

union sau_state {
	struct {
		ucontext_t	ss_ctx;
		struct sa_t	ss_sa;
	} ss_captured;
	struct {
		struct proc	*ss_proc;
	} ss_deferred;
};

struct sadata_upcall {
	SIMPLEQ_ENTRY(sadata_upcall)	sau_next;
	int								sau_flags;
	int								sau_type;
	size_t							sau_argsize;
	void							*sau_arg;
	void							(*sau_argfreefunc)(void *);
	stack_t							sau_stack;
	union sau_state					sau_event;
	union sau_state					sau_interrupted;
};

#define	SAU_FLAG_DEFERRED_EVENT			0x1
#define	SAU_FLAG_DEFERRED_INTERRUPTED	0x2

#define	SA_UPCALL_TYPE_MASK				0x00FF

#define	SA_UPCALL_DEFER_EVENT			0x1000
#define	SA_UPCALL_DEFER_INTERRUPTED		0x2000
#define	SA_UPCALL_DEFER					(SA_UPCALL_DEFER_EVENT | SA_UPCALL_DEFER_INTERRUPTED)

struct sastack {
	stack_t							sast_stack;
	SPLAY_ENTRY(sastack)			sast_node;
	unsigned int					sast_gen;
};

struct sadata_vp {
	int								savp_id;				/* "virtual processor" identifier */
	SLIST_ENTRY(sadata_vp)			savp_next; 				/* link to next sadata_vp */
	struct lock_object				savp_lock; 				/* lock on these fields */
	//struct proc						*savp_proc;				/* proc on "virtual processor" */
	//struct proc						*savp_blocker;			/* recently blocked proc */
	//struct proc						*savp_wokenq_head; 		/* list of woken procs */
	//struct proc						**savp_wokenq_tailp; 	/* list of woken procs */
	vm_offset_t						savp_faultaddr;			/* page fault address */
	vm_offset_t						savp_ofaultaddr;		/* old page fault address */
	LIST_HEAD(, proc)				savp_proccache; 		/* list of available procs */
	int								savp_ncached;			/* list length */
	SIMPLEQ_HEAD(, sadata_upcall)	savp_upcalls; 			/* pending upcalls */

	struct thread					*savp_thread;			/* thread on "virtual processor" */
	struct thread					*savp_blocker;			/* recently blocked thread */
	struct thread					*savp_wokenq_head; 		/* list of woken thread */
	struct thread					**savp_wokenq_tailp; 	/* list of woken thread */
};

struct sadata {
	struct lock_object 				sa_lock;				/* lock on these fields */
	int								sa_flag;				/* SA_* flags */
	sa_upcall_t						sa_upcall;				/* upcall entry point */
	int								sa_concurrency;			/* current concurrency */
	int								sa_maxconcurrency;		/* requested concurrency */
	SPLAY_HEAD(sasttree, sastack) 	sa_stackstree; 			/* tree of upcall stacks */
	struct sastack					*sa_stacknext;			/* next free stack */
	ssize_t							sa_stackinfo_offset;	/* offset from ss_sp to stackinfo data */
	int								sa_nstacks;				/* number of upcall stacks */
	SLIST_HEAD(, sadata_vp)			sa_vps;					/* list of "virtual processors" */
};

#define SA_FLAG_ALL	SA_FLAG_PREEMPT

#define	SA_MAXNUMSTACKS	16			/* Maximum number of upcall stacks per VP. */

struct sadata_upcall *sadata_upcall_alloc(int);
void				sadata_upcall_free(struct sadata_upcall *);

void		sa_release(struct proc *);
void		sa_switch(struct proc *, int);
void		sa_preempt(struct proc *);
void		sa_yield(struct proc *);
int			sa_upcall(struct proc *, int, struct proc *, struct proc *, size_t, void *);

void		sa_putcachelwp(struct proc *, struct lwp *);
struct lwp 	*sa_getcachelwp(struct sadata_vp *);


void		sa_unblock_userret(struct proc *);
void		sa_upcall_userret(struct proc *);
void		cpu_upcall(struct proc *, int, int, int, void *, void *, void *, sa_upcall_t);

#endif /* !_SYS_SAVAR_H_ */
