/*	$NetBSD: machdep.c,v 1.552.2.3 2004/08/16 17:46:05 jmc Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/proc.h>
#include <sys/signal.h>

#include <arch/i386/include/frame.h>
#include <arch/i386/include/npx.h>
#include <arch/i386/include/pcb.h>
#include <arch/i386/include/proc.h>
#include <arch/i386/include/psl.h>
#include <arch/i386/include/vm86.h>

#include <devel/sys/ucontext.h>
#include <devel/arch/i386/include/mcontext.h>

int
cpu_getmcontext(p, mcp, flags)
	struct proc *p;
	mcontext_t *mcp;
	unsigned int flags;
{
	const struct trapframe *tf;
	gregset_t *gr;

	tf = p->p_md.md_regs;
	gr = mcp->mc_gregs;
	/* Save register context. */
	if (tf->tf_eflags & PSL_VM) {
		struct trapframe_vm86 *tf = (struct trapframe_vm86 *) tf;
		struct vm86_kernel *vm86 =  &p->p_addr->u_pcb.pcb_vm86;

		gr->mc_gs  = tf->tf_vm86_gs;
		gr->mc_fs  = tf->tf_vm86_fs;
		gr->mc_es  = tf->tf_vm86_es;
		gr->mc_ds  = tf->tf_vm86_ds;
		gr->mc_eflags = (tf->tf_eflags & ~(PSL_VIF | PSL_VIP)) | (vm86->vm86_eflags & (PSL_VIF | PSL_VIP));
	}  else {
		gr->mc_gs = tf->tf_gs;
		gr->mc_fs = tf->tf_fs;
		gr->mc_es = tf->tf_es;
		gr->mc_ds = tf->tf_ds;
		gr->mc_eflags = tf->tf_eflags;
	}
	gr->mc_edi = tf->tf_edi;
	gr->mc_esi = tf->tf_esi;
	gr->mc_ebp = tf->tf_ebp;
	gr->mc_ebx = tf->tf_ebx;
	gr->mc_edx = tf->tf_edx;
	gr->mc_ecx = tf->tf_ecx;
	gr->mc_eax = tf->tf_eax;
	gr->mc_eip = tf->tf_eip;
	gr->mc_cs  = tf->tf_cs;
	gr->mc_esp = tf->tf_esp;
	gr->mc_ss  = tf->tf_ss;
	gr->mc_trapno = tf->tf_trapno;
	gr->mc_err = tf->tf_err;

	*flags |= _UC_CPU;

	/* Save floating point register context, if any. */
	if ((p->p_md.md_flags & MDP_USEDFPU) != 0) {
		if (p->p_addr->u_pcb.pcb_fpcpu) {
			npxsave_proc(p, 1);
		}
		if (i386_use_fxsave) {
			memcpy(&mcp->mc_fpregs.mc_fp_reg_set.mc_fp_xmm_state.fp_xmm,
					&p->p_addr->u_pcb.pcb_savefpu.sv_fx,
					sizeof(mcp->mc_fpregs.mc_fp_reg_set.mc_fp_xmm_state.fp_xmm));
			*flags |= _UC_FXSAVE;
		} else {
			memcpy(&mcp->mc_fpregs.mc_fp_reg_set.mc_fpchip_state.fp_state,
					&p->p_addr->u_pcb.pcb_savefpu.sv_87,
					sizeof(mcp->mc_fpregs.mc_fp_reg_set.mc_fpchip_state.fp_state));
		}
		*flags |= _UC_FPU;
	}
	return (0);
}

int
cpu_setmcontext(p, mcp, flags)
	struct proc *p;
	const mcontext_t *mcp;
	unsigned int flags;
{
	struct trapframe *tf;
	gregset_t *gr;

	tf = p->p_md.md_regs;
	gr = mcp->mc_gregs;
	/* Restore register context, if any. */
	if ((flags & _UC_CPU) != 0) {
		if (gr->mc_eflags & PSL_VM) {
			struct trapframe_vm86 *tf = (struct trapframe_vm86*) tf;
			struct vm86_kernel *vm86 = &p->p_addr->u_pcb.pcb_vm86;

			tf->tf_vm86_gs = gr->mc_gs;
			tf->tf_vm86_fs = gr->mc_fs;
			tf->tf_vm86_es = gr->mc_es;
			tf->tf_vm86_ds = gr->mc_ds;
			tf->tf_eflags  = (gr->mc_eflags & ~(PSL_VIF | PSL_VIP)) | (vm86->vm86_eflags & (PSL_VIF | PSL_VIP));
		} else {
			/*
			 * Check for security violations.  If we're returning
			 * to protected mode, the CPU will validate the segment
			 * registers automatically and generate a trap on
			 * violations.  We handle the trap, rather than doing
			 * all of the checking here.
			 */
			if (((gr->mc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC)
					|| !USERMODE(gr->mc_cs, gr->mc_eflags)) {
				printf("cpu_setmcontext error: uc EFL: 0x%08x"
						" tf EFL: 0x%08x uc CS: 0x%x\n", gr->mc_eflags,
						tf->tf_eflags, gr->mc_cs);
				return (EINVAL);
			}
			tf->tf_gs = gr->mc_gs;
			tf->tf_fs = gr->mc_fs;
			tf->tf_es = gr->mc_es;
			tf->tf_ds = gr->mc_ds;
			/* Only change the user-alterable part of eflags */
			tf->tf_eflags &= ~PSL_USER;
			tf->tf_eflags |= (gr->mc_eflags & PSL_USER);
		}
	}
	tf->tf_edi = gr->mc_edi;
	tf->tf_esi = gr->mc_esi;
	tf->tf_ebp = gr->mc_ebp;
	tf->tf_ebx = gr->mc_ebx;
	tf->tf_edx = gr->mc_edx;
	tf->tf_ecx = gr->mc_ecx;
	tf->tf_eax = gr->mc_eax;
	tf->tf_eip = gr->mc_eip;
	tf->tf_cs  = gr->mc_cs;
	tf->tf_esp = gr->mc_esp;
	tf->tf_ss  = gr->mc_ss;

	if (flags & _UC_SETSTACK) {
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	}
	if (flags & _UC_CLRSTACK) {
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	}

	return (0);
}
