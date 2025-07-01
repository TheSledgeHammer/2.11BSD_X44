/*	$NetBSD: kern_sig.c,v 1.189.2.6.4.1 2005/12/29 01:32:51 riz Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_sig.c	8.14 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysdecl.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ucontext.h>
#include <sys/wait.h>

void
proc_getucontext(p, ucp)
	struct proc *p;
	ucontext_t *ucp;
{
	ucp->uc_flags = 0;
	ucp->uc_link = p->p_ctxlink;

	(void)sigprocmask(0, NULL, &ucp->uc_sigmask);
	ucp->uc_flags |= _UC_SIGMASK;

	/*
	 * The (unsupplied) definition of the `current execution stack'
	 * in the System V Interface Definition appears to allow returning
	 * the main context stack.
	 */
	if ((p->p_sigacts->ps_sigstk.ss_flags & SS_ONSTACK) == 0) {
		ucp->uc_stack.ss_base = (void *)USRSTACK;
		ucp->uc_stack.ss_size = ctob(p->p_vmspace->vm_ssize);
		ucp->uc_stack.ss_flags = 0;	/* XXX, def. is Very Fishy */
	} else {
		/* Simply copy alternate signal execution stack. */
		ucp->uc_stack = p->p_sigacts->ps_sigstk;
	}
	ucp->uc_flags |= _UC_STACK;

	cpu_getmcontext(p, &ucp->uc_mcontext, &ucp->uc_flags);
}

int
proc_setucontext(p, ucp)
	struct proc *p;
	const ucontext_t *ucp;
{
	int		error;

	if ((error = cpu_setmcontext(p, &ucp->uc_mcontext, ucp->uc_flags)) != 0) {
		return (error);
	}
	p->p_ctxlink = ucp->uc_link;

	if ((ucp->uc_flags & _UC_SIGMASK) != 0) {
		sigprocmask(SIG_SETMASK, &ucp->uc_sigmask, NULL);
	}

	/*
	 * If there was stack information, update whether or not we are
	 * still running on an alternate signal stack.
	 */
	if ((ucp->uc_flags & _UC_STACK) != 0) {
		if (ucp->uc_stack.ss_flags & SS_ONSTACK) {
			p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
		} else {
			p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
		}
	}

	return (0);
}

int
getcontext()
{
	register struct getcontext_args {
		syscallarg(struct __ucontext *) ucp;
	} *uap = (struct getcontext_args *)u.u_ap;
	ucontext_t uc;

	proc_getucontext(u.u_procp, &uc);

	return (copyout(&uc, SCARG(uap, ucp), sizeof (*SCARG(uap, ucp))));
}

int
setcontext()
{
	register struct setcontext_args {
		syscallarg(struct __ucontext *) ucp;
	} *uap = (struct setcontext_args *)u.u_ap;
	ucontext_t uc;
	int error;

	if (SCARG(uap, ucp) == NULL) {
		exit(W_EXITCODE(0, 0));
	} else if ((error = copyin(SCARG(uap, ucp), &uc, sizeof(uc))) != 0
			|| (error = proc_setucontext(u.u_procp, &uc)) != 0) {
		return (error);
	}
	return (EJUSTRETURN);
}
