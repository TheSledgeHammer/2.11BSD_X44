/*	$NetBSD: mcontext.h,v 1.6 2003/10/08 22:43:01 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#ifndef _I386_MCONTEXT_H_
#define _I386_MCONTEXT_H_

/*
 * mcontext extensions to handle signal delivery.
 */
#define _UC_SETSTACK	0x00010000
#define _UC_CLRSTACK	0x00020000
#define _UC_VM			0x00040000

/*
 * Layout of mcontext_t according to the System V Application Binary Interface,
 * Intel386(tm) Architecture Processor Supplement, Fourth Edition.
 */

//#ifdef __i386__
/* Keep _MC_* values similar to amd64 */
#define	_MC_HASSEGS		0x1
#define	_MC_HASBASES	0x2
#define	_MC_HASFPXSTATE	0x4
#define	_MC_FLAG_MASK	(_MC_HASSEGS | _MC_HASBASES | _MC_HASFPXSTATE)

struct mcontext {
	/*
	 * The definition of mcontext_t must match the layout of
	 * struct sigcontext after the sc_mask member.  This is so
	 * that we can support sigcontext and ucontext_t at the same
	 * time.
	 */
	__register_t	mc_onstack;	/* XXX - sigcontext compat. */
	__register_t	mc_gs;		/* machine state (struct trapframe) */
	__register_t	mc_fs;
	__register_t	mc_es;
	__register_t	mc_ds;
	__register_t	mc_edi;
	__register_t	mc_esi;
	__register_t	mc_ebp;
	__register_t	mc_isp;
	__register_t	mc_ebx;
	__register_t	mc_edx;
	__register_t	mc_ecx;
	__register_t	mc_eax;
	__register_t	mc_trapno;
	__register_t	mc_err;
	__register_t	mc_eip;
	__register_t	mc_cs;
	__register_t	mc_eflags;
	__register_t	mc_esp;
	__register_t	mc_ss;

	int				mc_len;			/* sizeof(mcontext_t) */
#define	_MC_FPFMT_NODEV		0x10000	/* device not present or configured */
#define	_MC_FPFMT_387		0x10001
#define	_MC_FPFMT_XMM		0x10002
	int				mc_fpformat;
#define	_MC_FPOWNED_NONE	0x20000	/* FP state not used */
#define	_MC_FPOWNED_FPU		0x20001	/* FP state came from FPU */
#define	_MC_FPOWNED_PCB		0x20002	/* FP state came from PCB */
	int				mc_ownedfp;
	__register_t 	mc_flags;
	/*
	 * See <machine/npx.h> for the internals of mc_fpstate[].
	 */
	int				mc_fpstate[128];// __aligned(16);

	__register_t 	mc_fsbase;
	__register_t 	mc_gsbase;

	__register_t 	mc_xfpustate;
	__register_t 	mc_xfpustate_len;

	int				mc_spare2[4];
};
typedef struct mcontext mcontext_t;

//#endif /* __i386__ */

#endif	/* !_I386_MCONTEXT_H_ */
