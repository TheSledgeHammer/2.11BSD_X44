/*	$NetBSD: ucontext.h,v 1.9 2006/08/20 08:02:21 skrll Exp $	*/

/*-
 * Copyright (c) 1999, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein, and by Jason R. Thorpe.
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

#ifndef _SYS_UCONTEXT_H_
#define _SYS_UCONTEXT_H_

//#include <sys/sigtypes.h>
#include <devel/arch/i386/include/mcontext.h>

typedef struct {
	void			*ss_sp;			/* signal stack base */
	size_t			ss_size;		/* signal stack length */
	int				ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
} stack_t;


struct __ucontext {
	unsigned int	uc_flags;		/* properties */
	ucontext_t * 	uc_link;		/* context to resume */
	sigset_t		uc_sigmask;		/* signals blocked in this context */
	stack_t			uc_stack;		/* the stack used by this context */
	mcontext_t		uc_mcontext;	/* machine state */
#if defined(_UC_MACHINE_PAD)
	long			__uc_pad[_UC_MACHINE_PAD];
#endif
};
typedef struct __ucontext	ucontext_t;

#ifndef _UC_UCONTEXT_ALIGN
#define _UC_UCONTEXT_ALIGN (~0)
#endif

/* uc_flags */
#define _UC_SIGMASK	0x01			/* valid uc_sigmask */
#define _UC_STACK	0x02			/* valid uc_stack */
#define _UC_CPU		0x04			/* valid GPR context in uc_mcontext */
#define _UC_FPU		0x08			/* valid FPU context in uc_mcontext */

#ifdef _KERNEL
struct proc;

void	getucontext(struct proc *, ucontext_t *);
int		setucontext(struct proc *, const ucontext_t *);
void	cpu_getmcontext(struct proc *, mcontext_t *, unsigned int *);
int		cpu_setmcontext(struct proc *, const mcontext_t *, unsigned int);
#endif /* _KERNEL */
#endif /* !_SYS_UCONTEXT_H_ */
