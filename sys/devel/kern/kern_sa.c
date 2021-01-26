/*	$NetBSD: kern_sa.c,v 1.87 2006/11/01 10:17:58 yamt Exp $	*/

/*-
 * Copyright (c) 2001, 2004, 2005 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

/* __KERNEL_RCSID(0, "$NetBSD: kern_sa.c,v 1.87 2006/11/01 10:17:58 yamt Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>
#include <sys/user.h>

#include <vm/include/vm_extern.h>

#include <devel/sys/sa.h>
#include <devel/sys/savar.h>
#include <devel/sys/ucontext.h>
#include <devel/sys/malloctypes.h>

#define M_SA 77 /* scheduler activation (Malloc type) */

static struct sadata_vp 		*sa_newsavp(struct sadata *);
static inline int 				sa_stackused(struct sastack *, struct sadata *);
static inline void 				sa_setstackfree(struct sastack *, struct sadata *);
static struct sastack 			*sa_getstack(struct sadata *);
static inline struct sastack 	*sa_getstack0(struct sadata *);
static inline int 				sast_compare(struct sastack *, struct sastack *);
#ifdef SMP
static int 						sa_increaseconcurrency(struct lwp *, int);
#endif
static void 					sa_setwoken(struct lwp *);
static void 					sa_switchcall(void *);
static int 						sa_newcachelwp(struct lwp *);
static inline void 				sa_makeupcalls(struct lwp *);
static struct lwp 				*sa_vp_repossess(struct lwp *l);

static inline int 				sa_pagefault(struct lwp *, ucontext_t *);

static void 					sa_upcall0(struct sadata_upcall *, int, struct lwp *, struct lwp *, size_t, void *, void (*)(void *));
static void 					sa_upcall_getstate(union sau_state *, struct lwp *);

#define SA_DEBUG

#ifdef SA_DEBUG
#define DPRINTF(x)		do { if (sadebug) printf_nolog x; } while (0)
#define DPRINTFN(n,x)	do { if (sadebug & (1<<(n-1))) printf_nolog x; } while (0)
int	sadebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define SA_PROC_STATE_LOCK(p, f) do {		\
	(f) = (p)->p_flag;     					\
	(p)->p_flag &= ~L_SA;					\
} while (/*CONSTCOND*/ 0)

#define SA_PROC_STATE_UNLOCK(p, f) do {		\
	(p)->p_flag |= (f) & L_SA;				\
} while (/*CONSTCOND*/ 0)


struct sadata_upcall 	saupcall;
struct sadata 			sadata;
struct sastack			sastack;
struct sadata_vp		savp;

void
sa_init()
{
	MALLOC(&saupcall, struct sadata_upcall *, sizeof(struct sadata_upcall *), M_SA, M_WAITOK | M_NOWAIT);
	MALLOC(&sadata, struct sadata *, sizeof(struct sadata *), M_SA, M_WAITOK | M_NOWAIT);
	MALLOC(&sastack, struct sastack *, sizeof(struct sastack *), M_SA, M_WAITOK | M_NOWAIT);
	MALLOC(&savp, struct sadata_vp *, sizeof(struct sadata_vp *), M_SA, M_WAITOK | M_NOWAIT);
}

/*
 * sadata_upcall_alloc:
 *
 *	Allocate an sadata_upcall structure.
 */
struct sadata_upcall *
sadata_upcall_alloc(int waitok)
{
	struct sadata_upcall *sau;

	sau = &saupcall;//(struct sadata_upcall *)malloc(sizeof(struct sadata_upcall), M_SA, waitok ? M_WAITOK : M_NOWAIT);
	if (sau) {
		sau->sau_arg = NULL;
	}
	return (sau);
}

/*
 * sadata_upcall_free:
 *
 *	Free an sadata_upcall structure and any associated argument data.
 */
void
sadata_upcall_free(struct sadata_upcall *sau)
{

	if (sau == NULL) {
		return;
	}
	if (sau->sau_arg) {
		(*sau->sau_argfreefunc)(sau->sau_arg);
	}
	free(sau, M_SA);
}

static struct sadata_vp *
sa_newsavp(struct sadata *sa)
{
	struct sadata_vp *vp, *qvp;

	/* Allocate virtual processor data structure */
	vp = &sadata;
	/* Initialize. */
	memset(vp, 0, sizeof(*vp));
	simple_lock_init(&vp->savp_lock);
	vp->savp_proc = NULL;
	vp->savp_wokenq_head = NULL;
	vp->savp_faultaddr = 0;
	vp->savp_ofaultaddr = 0;
	LIST_INIT(&vp->savp_proccache);
	vp->savp_ncached = 0;
	SIMPLEQ_INIT(&vp->savp_upcalls);

	simple_lock(&sa->sa_lock);
	/* find first free savp_id and add vp to sorted slist */
	if (LIST_EMPTY(&sa->sa_vps) ||
	    LIST_FIRST(&sa->sa_vps)->savp_id != 0) {
		vp->savp_id = 0;
		LIST_INSERT_HEAD(&sa->sa_vps, vp, savp_next);
	} else {
		LIST_FOREACH(qvp, &sa->sa_vps, savp_next) {
			if (LIST_NEXT(qvp, savp_next) == NULL ||
			    LIST_NEXT(qvp, savp_next)->savp_id !=
			    qvp->savp_id + 1)
				break;
		}
		vp->savp_id = qvp->savp_id + 1;
		LIST_INSERT_AFTER(qvp, vp, savp_next);
	}
	simple_unlock(&sa->sa_lock);

	return (vp);
}

int
sys_sa_register(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_register_args /* {
		syscallarg(sa_upcall_t) new;
		syscallarg(sa_upcall_t *) old;
		syscallarg(int) flags;
		syscallarg(ssize_t) stackinfo_offset;
	} */ *uap = v;
	int error;
	sa_upcall_t prev;

	error = dosa_register(l, SCARG(uap, new), &prev, SCARG(uap, flags),
	    SCARG(uap, stackinfo_offset));
	if (error)
		return error;

	if (SCARG(uap, old))
		return copyout(&prev, SCARG(uap, old),
		    sizeof(prev));
	return 0;
}

int
dosa_register(struct proc *p, sa_upcall_t new, sa_upcall_t *prev, int flags, ssize_t stackinfo_offset)
{
	struct sadata *sa;

	if (p->p_sa == NULL) {
		/* Allocate scheduler activations data structure */
		sa = pool_get(&sadata_pool, PR_WAITOK);
		/* Initialize. */
		memset(sa, 0, sizeof(*sa));
		simple_lock_init(&sa->sa_lock);
		sa->sa_flag = flags & SA_FLAG_ALL;
		sa->sa_maxconcurrency = 1;
		sa->sa_concurrency = 1;
		SPLAY_INIT(&sa->sa_stackstree);
		sa->sa_stacknext = NULL;
		if (flags & SA_FLAG_STACKINFO)
			sa->sa_stackinfo_offset = stackinfo_offset;
		else
			sa->sa_stackinfo_offset = 0;
		sa->sa_nstacks = 0;
		LIST_INIT(&sa->sa_vps);
		p->p_sa = sa;
		KASSERT(l->l_savp == NULL);
	}
	if (l->l_savp == NULL) {
		l->l_savp = sa_newsavp(p->p_sa);
		sa_newcachelwp(l);
	}

	*prev = p->p_sa->sa_upcall;
	p->p_sa->sa_upcall = new;

	return (0);
}

void
sa_release(struct proc *p)
{
	struct sadata *sa;
	struct sastack *sast, *next;
	struct sadata_vp *vp;
	struct lwp *l;

	sa = p->p_sa;
	KDASSERT(sa != NULL);
	KASSERT(p->p_nlwps <= 1);

	for (sast = SPLAY_MIN(sasttree, &sa->sa_stackstree); sast != NULL;
	     sast = next) {
		next = SPLAY_NEXT(sasttree, &sa->sa_stackstree, sast);
		SPLAY_REMOVE(sasttree, &sa->sa_stackstree, sast);
		pool_put(&sastack_pool, sast);
	}

	p->p_flag &= ~P_SA;
	while ((vp = LIST_FIRST(&p->p_sa->sa_vps)) != NULL) {
		LIST_REMOVE_HEAD(&p->p_sa->sa_vps, savp_next);
		pool_put(&savp_pool, vp);
	}
	pool_put(&sadata_pool, sa);
	p->p_sa = NULL;
	l = LIST_FIRST(&p->p_lwps);
	if (l) {
		KASSERT(LIST_NEXT(l, l_sibling) == NULL);
		l->l_savp = NULL;
	}
}

static int
sa_fetchstackgen(struct sastack *sast, struct sadata *sa, unsigned int *gen)
{
	int error;

	/* COMPAT_NETBSD32:  believe it or not, but the following is ok */
	error = copyin(&((struct sa_stackinfo_t *)
	    ((char *)sast->sast_stack.ss_sp +
	    sa->sa_stackinfo_offset))->sasi_stackgen, gen, sizeof(*gen));

	return error;
}

static inline int
sa_stackused(struct sastack *sast, struct sadata *sa)
{
	unsigned int gen;

	if (sa_fetchstackgen(sast, sa, &gen)) {
#ifdef DIAGNOSTIC
		printf("sa_stackused: couldn't copyin sasi_stackgen");
#endif
		sigexit(curlwp, SIGILL);
		/* NOTREACHED */
	}
	return (sast->sast_gen != gen);
}

static inline void
sa_setstackfree(struct sastack *sast, struct sadata *sa)
{
	unsigned int gen;

	if (sa_fetchstackgen(sast, sa, &gen)) {
#ifdef DIAGNOSTIC
		printf("sa_setstackfree: couldn't copyin sasi_stackgen");
#endif
		sigexit(curproc, SIGILL);
		/* NOTREACHED */
	}
	sast->sast_gen = gen;
}

/*
 * Find next free stack, starting at sa->sa_stacknext.
 */
static struct sastack *
sa_getstack(struct sadata *sa)
{
	struct sastack *sast;

	SCHED_ASSERT_UNLOCKED();

	if ((sast = sa->sa_stacknext) == NULL || sa_stackused(sast, sa))
		sast = sa_getstack0(sa);

	if (sast == NULL)
		return NULL;

	sast->sast_gen++;

	return sast;
}

static inline struct sastack *
sa_getstack0(struct sadata *sa)
{
	struct sastack *start;

	if (sa->sa_stacknext == NULL) {
		sa->sa_stacknext = SPLAY_MIN(sasttree, &sa->sa_stackstree);
		if (sa->sa_stacknext == NULL)
			return NULL;
	}
	start = sa->sa_stacknext;

	while (sa_stackused(sa->sa_stacknext, sa)) {
		sa->sa_stacknext = SPLAY_NEXT(sasttree, &sa->sa_stackstree,
		    sa->sa_stacknext);
		if (sa->sa_stacknext == NULL)
			sa->sa_stacknext = SPLAY_MIN(sasttree,
			    &sa->sa_stackstree);
		if (sa->sa_stacknext == start)
			return NULL;
	}
	return sa->sa_stacknext;
}

static inline int
sast_compare(struct sastack *a, struct sastack *b)
{
	if (a->sast_stack.ss_sp + a->sast_stack.ss_size <= b->sast_stack.ss_sp) {
		return (-1);
	} else if (a->sast_stack.ss_sp >= b->sast_stack.ss_sp + b->sast_stack.ss_size) {
		return (1);
	}
	return (0);
}

static int
sa_copyin_stack(stack_t *stacks, int index, stack_t *dest)
{
	return copyin(stacks + index, dest, sizeof(stack_t));
}

int
sys_sa_stacks(struct proc *p, void *v, register_t *retval)
{
	struct sys_sa_stacks_args /* {
		syscallarg(int) num;
		syscallarg(stack_t *) stacks;
	} */ *uap = v;

	return sa_stacks1(p, retval, SCARG(uap, num), SCARG(uap, stacks), sa_copyin_stack);
}

int
sa_stacks1(struct proc *p, register_t *retval, int num, stack_t *stacks, sa_copyin_stack_t do_sa_copyin_stack)
{
	struct sadata *sa = p->p_sa;
	struct sastack *sast, newsast;
	int count, error, f, i;

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	count = num;
	if (count < 0)
		return (EINVAL);

	SA_PROC_STATE_LOCK(p, f);

	error = 0;

	for (i = 0; i < count; i++) {
		error = do_sa_copyin_stack(stacks, i, &newsast.sast_stack);
		if (error) {
			count = i;
			break;
		}
		sast = SPLAY_FIND(sasttree, &sa->sa_stackstree, &newsast);
		if (sast != NULL) {
			DPRINTFN(9,
					("sa_stacks(%d.%d) returning stack %p\n", p->p_pid, l->l_lid, newsast.sast_stack.ss_sp));
			if (sa_stackused(sast, sa) == 0) {
				count = i;
				error = EEXIST;
				break;
			}
		} else if (sa->sa_nstacks >=
		SA_MAXNUMSTACKS * sa->sa_concurrency) {
			DPRINTFN(9,
					("sa_stacks(%d.%d) already using %d stacks\n", p->p_pid, l->l_lid, SA_MAXNUMSTACKS * sa->sa_concurrency));
			count = i;
			error = ENOMEM;
			break;
		} else {
			DPRINTFN(9,
					("sa_stacks(%d.%d) adding stack %p\n", p->p_pid, l->l_lid, newsast.sast_stack.ss_sp));
			sast = pool_get(&sastack_pool, PR_WAITOK);
			sast->sast_stack = newsast.sast_stack;
			SPLAY_INSERT(sasttree, &sa->sa_stackstree, sast);
			sa->sa_nstacks++;
		}
		sa_setstackfree(sast, sa);
	}

	SA_LWP_STATE_UNLOCK(p, f);

	*retval = count;
	return (error);
}

