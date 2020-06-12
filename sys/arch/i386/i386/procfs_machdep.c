/*	$NetBSD: process_machdep.c,v 1.94 2019/08/06 02:04:43 kamil Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2001, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by Jason R. Thorpe of Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * This file may seem a bit stylized, but that so that it's easier to port.
 * Functions to be implemented here are:
 *
 * process_read_regs(proc, regs)
 *	Get the current user-visible register set from the process
 *	and copy it into the regs structure (<machine/reg.h>).
 *	The process is stopped at the time read_regs is called.
 *
 * process_write_regs(proc, regs)
 *	Update the current register set from the passed in regs
 *	structure.  Take care to avoid clobbering special CPU
 *	registers or privileged bits in the PSL.
 *	The process is stopped at the time write_regs is called.
 *
 * process_read_fpregs(proc, regs, sz)
 *	Get the current user-visible register set from the process
 *	and copy it into the regs structure (<machine/reg.h>).
 *	The process is stopped at the time read_fpregs is called.
 *
 * process_write_fpregs(proc, regs, sz)
 *	Update the current register set from the passed in regs
 *	structure.  Take care to avoid clobbering special CPU
 *	registers or privileged bits in the PSL.
 *	The process is stopped at the time write_fpregs is called.
 *
 * process_read_dbregs(proc, regs)
 *	Get the current user-visible register set from the process
 *	and copy it into the regs structure (<machine/reg.h>).
 *	The process is stopped at the time read_dbregs is called.
 *
 * process_write_dbregs(proc, regs)
 *	Update the current register set from the passed in regs
 *	structure.  Take care to avoid clobbering special CPU
 *	registers or privileged bits in the PSL.
 *	The process is stopped at the time write_dbregs is called.
 *
 * process_sstep(proc)
 *	Arrange for the process to trap after executing a single instruction.
 *
 * process_set_pc(proc)
 *	Set the process's program counter.
 *
 */

#include <sys/cdefs.h>
/*__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.94 2019/08/06 02:04:43 kamil Exp $");*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>

#include <vm/include/vm_extern.h>

#include <machine/proc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/segments.h>

static inline struct trapframe *
process_frame(p)
	struct proc *p;
{
	return (p->p_md.md_regs);
}

static inline struct save87  *
process_fpframe(p)
	struct proc *p;
{
	return (p->p_addr->u_pcb.pcb_savefpu);
}

int
process_read_regs(p, regs)
	struct proc *p;
	const struct reg32 *regs;
{
	struct trapframe *tf = process_frame(p);

	regs->r_gs = tf->tf_gs & 0xffff;
	regs->r_fs = tf->tf_fs & 0xffff;
	regs->r_es = tf->tf_es & 0xffff;
	regs->r_ds = tf->tf_ds & 0xffff;
	regs->r_eflags = tf->tf_eflags;

	regs->r_edi = tf->tf_edi;
	regs->r_esi = tf->tf_esi;
	regs->r_ebp = tf->tf_ebp;
	regs->r_ebx = tf->tf_ebx;
	regs->r_edx = tf->tf_edx;
	regs->r_ecx = tf->tf_ecx;
	regs->r_eax = tf->tf_eax;
	regs->r_eip = tf->tf_eip;
	regs->r_cs = tf->tf_cs & 0xffff;
	regs->r_esp = tf->tf_esp;
	regs->r_ss = tf->tf_ss & 0xffff;

	return (0);
}

int
process_read_fpregs(p, regs, sz)
	struct proc *p;
	struct fpreg *regs;
	size_t *sz;
{
	struct save87 *frame = process_fpframe(p);
	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		extern struct proc *npxproc;

		if (npxproc == p)
			npxsave();
#endif
	} else {
		u_short cw;
		memset(frame, 0, sizeof(*regs));

		p->p_md.md_flags |= MDP_USEDFPU;
	}
	memcpy(regs, frame, sizeof(*regs));
	return (0);
}

int
process_write_regs(p, regs)
	struct proc *p;
	const struct reg32 *regs;
{
	struct trapframe *tf = process_frame(p);

	/*
	 * Check for security violations.
	 */
	if (((regs->r_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
	    !USERMODE(regs->r_cs))
		return (EINVAL);

	tf->tf_gs = regs->r_gs;
	tf->tf_fs = regs->r_fs;
	tf->tf_es = regs->r_es;
	tf->tf_ds = regs->r_ds;
	tf->tf_eflags = regs->r_eflags;

	tf->tf_edi = regs->r_edi;
	tf->tf_esi = regs->r_esi;
	tf->tf_ebp = regs->r_ebp;
	tf->tf_ebx = regs->r_ebx;
	tf->tf_edx = regs->r_edx;
	tf->tf_ecx = regs->r_ecx;
	tf->tf_eax = regs->r_eax;
	tf->tf_eip = regs->r_eip;
	tf->tf_cs = regs->r_cs;
	tf->tf_esp = regs->r_esp;
	tf->tf_ss = regs->r_ss;

	return (0);
}

int
process_write_fpregs(p, regs, sz)
	struct proc *p;
	const struct fpreg *regs;
	size_t *sz;
{
	struct save87 *frame = process_fpframe(p);
	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		extern struct proc *npxproc;
		if (npxproc == p)
			npxdrop();

#endif
	} else {
		p->p_md.md_flags |= MDP_USEDFPU;
	}
	memcpy(frame, regs, sizeof(*regs));
	return (0);
}

int
process_sstep(p, sstep)
	struct proc *p;
	int sstep;
{
	struct trapframe *tf = process_frame(p);

	if (sstep)
		tf->tf_eflags |= PSL_T;
	else
		tf->tf_eflags &= ~PSL_T;

	return (0);
}

int
process_set_pc(p, addr)
	struct proc *p;
	void *addr;
{
	struct trapframe *tf = process_frame(p);

	tf->tf_eip = (int)addr;

	return (0);
}
