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

#include "npx.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>
#include <sys/errno.h>

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

static int
xmm_to_s87_tag(const uint8_t *fpac, int regno, uint8_t tw)
{
	static const uint8_t empty_significand[8] = { 0 };
	int tag;
	uint16_t exponent;

	if (tw & (1U << regno)) {
		exponent = fpac[8] | (fpac[9] << 8);
		switch (exponent) {
		case 0x7fff:
			tag = 2;
			break;

		case 0x0000:
			if (memcmp(empty_significand, fpac,
				   sizeof(empty_significand)) == 0)
				tag = 1;
			else
				tag = 2;
			break;

		default:
			if ((fpac[7] & 0x80) == 0)
				tag = 2;
			else
				tag = 0;
			break;
		}
	} else
		tag = 3;

	return (tag);
}

void
process_xmm_to_s87(const struct fxsave *sxmm, struct save87 *s87)
{
	int i;

	/* FPU control/status */
	s87->sv_env.en_cw = sxmm->fxv_env.fx_cw;
	s87->sv_env.en_sw = sxmm->fxv_env.fx_sw;
	/* tag word handled below */
	s87->sv_env.en_fip = sxmm->fxv_env.fx_fip;
	s87->sv_env.en_fcs = sxmm->fxv_env.fx_fcs;
	s87->sv_env.en_opcode = sxmm->fxv_env.fx_opcode;
	s87->sv_env.en_foo = sxmm->fxv_env.fx_foo;
	s87->sv_env.en_fos = sxmm->fxv_env.fx_fos;

	/* Tag word and registers. */
	s87->sv_env.en_tw = 0;
	s87->sv_ex_tw = 0;
	for (i = 0; i < 8; i++) {
		s87->sv_env.en_tw |=
		    (xmm_to_s87_tag(sxmm->fxv_ac[i].fp_bytes, i,
		     sxmm->fxv_env.fx_tw) << (i * 2));

		s87->sv_ex_tw |=
		    (xmm_to_s87_tag(sxmm->fxv_ac[i].fp_bytes, i,
		     sxmm->fxv_ex_tw) << (i * 2));

		memcpy(&s87->sv_ac[i].fp_bytes, &sxmm->fxv_ac[i].fp_bytes,
		    sizeof(s87->sv_ac[i].fp_bytes));
	}

	s87->sv_ex_sw = sxmm->fxv_ex_sw;
}

void
process_s87_to_xmm(const struct save87 *s87, struct fxsave *sxmm)
{
	int i;

	/* FPU control/status */
	sxmm->fxv_env.fx_cw = s87->sv_env.en_cw;
	sxmm->fxv_env.fx_sw = s87->sv_env.en_sw;
	/* tag word handled below */
	sxmm->fxv_env.fx_fip = s87->sv_env.en_fip;
	sxmm->fxv_env.fx_fcs = s87->sv_env.en_fcs;
	sxmm->fxv_env.fx_opcode = s87->sv_env.en_opcode;
	sxmm->fxv_env.fx_foo = s87->sv_env.en_foo;
	sxmm->fxv_env.fx_fos = s87->sv_env.en_fos;

	/* Tag word and registers. */
	for (i = 0; i < 8; i++) {
		if (((s87->sv_env.en_tw >> (i * 2)) & 3) == 3)
			sxmm->fxv_env.fx_tw &= ~(1U << i);
		else
			sxmm->fxv_env.fx_tw |= (1U << i);

#if 0
		/*
		 * Software-only word not provided by the userland fpreg
		 * structure.
		 */
		if (((s87->sv_ex_tw >> (i * 2)) & 3) == 3)
			sxmm->fxv_ex_tw &= ~(1U << i);
		else
			sxmm->fxv_ex_tw |= (1U << i);
#endif

		memcpy(&sxmm->fxv_ac[i].fp_bytes, &s87->sv_ac[i].fp_bytes,
		    sizeof(sxmm->fxv_ac[i].fp_bytes));
	}
#if 0
	/*
	 * Software-only word not provided by the userland fpreg
	 * structure.
	 */
	sxmm->fxv_ex_sw = s87->sv_ex_sw;
#endif
}

int
process_read_regs(p, regs)
	struct proc *p;
	const struct reg32 *regs;
{
	struct trapframe *tf = process_frame(p);

	if (tf->tf_eflags & PSL_VM) {
		struct trapframe_vm86 *tf = (struct trapframe_vm86 *) tf;

		regs->r_gs = tf->tf_vm86_gs;
		regs->r_fs = tf->tf_vm86_fs;
		regs->r_es = tf->tf_vm86_es;
		regs->r_ds = tf->tf_vm86_ds;
		regs->r_eflags = get_vflags(p);
	} else	{
		regs->r_gs = tf->tf_gs & 0xffff;
		regs->r_fs = tf->tf_fs & 0xffff;
		regs->r_es = tf->tf_es & 0xffff;
		regs->r_ds = tf->tf_ds & 0xffff;
		regs->r_eflags = tf->tf_eflags;
	}

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
process_read_fpregs(p, regs)
	struct proc *p;
	struct fpreg *regs;
{
	struct savefpu *frame = process_fpframe(p);

	if (p->p_md.md_flags & MDL_USEDFPU) {
#if NNPX > 0
		npxsave();
#endif
	} else {
		/*
		 * Fake a FNINIT.
		 * The initial control word was already set by setregs(), so
		 * save it temporarily.
		 */
		if (i386_use_fxsave) {
			uint32_t mxcsr = frame->sv_fx.fxv_env.fx_mxcsr;
			uint16_t cw = frame->sv_fx.fxv_env.fx_cw;

			/* XXX Don't zero XMM regs? */
			memset(&frame->sv_fx, 0, sizeof(frame->sv_fx));
			frame->sv_fx.fxv_env.fx_cw = cw;
			frame->sv_fx.fxv_env.fx_mxcsr = mxcsr;
			frame->sv_fx.fxv_env.fx_sw = 0x0000;
			frame->sv_fx.fxv_env.fx_tw = 0x00;
		} else {
			uint16_t cw = frame->sv_87.sv_env.en_cw;

			memset(&frame->sv_87, 0, sizeof(frame->sv_87));
			frame->sv_87.sv_env.en_cw = cw;
			frame->sv_87.sv_env.en_sw = 0x0000;
			frame->sv_87.sv_env.en_tw = 0xffff;
		}
		p->p_md.md_flags |= MDL_USEDFPU;
	}

	if (i386_use_fxsave) {
		struct save87 s87;

		/* XXX Yuck */
		process_xmm_to_s87(&frame->sv_fx, &s87);
		memcpy(regs, &s87, sizeof(*regs));
	} else
		memcpy(regs, &frame->sv_87, sizeof(*regs));
	return (0);
}

int
process_write_regs(p, regs)
	struct proc *p;
	const struct reg32 *regs;
{
	struct trapframe *tf = process_frame(p);

	if (regs->r_eflags & PSL_VM) {
		struct trapframe_vm86 *tf = (struct trapframe_vm86 *)tf;

		tf->tf_vm86_gs = regs->r_gs;
		tf->tf_vm86_fs = regs->r_fs;
		tf->tf_vm86_es = regs->r_es;
		tf->tf_vm86_ds = regs->r_ds;
		set_vflags(p, regs->r_eflags);

		/*
		 * Make sure that attempts at system calls from vm86
		 * mode die horribly.
		 */
		//p->p_md.md_syscall = syscall_vm86;
	} else	{
		/*
		 * Check for security violations.
		 */
		if (((regs->r_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0
				|| !USERMODE(regs->r_cs, regs->r_eflags))
			return (EINVAL);

		tf->tf_gs = regs->r_gs;
		tf->tf_fs = regs->r_fs;
		tf->tf_es = regs->r_es;
		tf->tf_ds = regs->r_ds;

		/* Restore normal syscall handler */
		if (tf->tf_eflags & PSL_VM)
			(*p->p_emul->e_syscall)(p);

		tf->tf_eflags = regs->r_eflags;
	}
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
	struct savefpu  *frame = process_fpframe(p);
	if (p->p_md.md_flags & MDP_USEDFPU) {
#if NNPX > 0
		extern struct proc *npxproc;
		if (npxproc == p)
			npxsave();

#endif
	} else {
		p->p_md.md_flags |= MDP_USEDFPU;
	}
	if (i386_use_fxsave) {
		struct save87 s87;

		/* XXX Yuck. */
		memcpy(&s87, regs, sizeof(*regs));
		process_s87_to_xmm(&s87, &frame->sv_fx);
	} else
		memcpy(&frame->sv_87, regs, sizeof(*regs));
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
