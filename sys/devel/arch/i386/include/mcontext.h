/*	$NetBSD: mcontext.h,v 1.7 2008/04/28 20:23:24 martin Exp $	*/

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
 * General register state
 */
struct gregset {
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

	__register_t 	mc_fsbase;
	__register_t 	mc_gsbase;
};
typedef struct gregset gregset_t;

struct fpregset {
	union {
		struct {
			int 	__fp_state[27]; /* Environment and registers */
			int 	__fp_status; 	/* Software status word */
		} __fpchip_state;
		struct {
			char 	__fp_emul[246];
			char 	__fp_epad[2];
		} __fp_emul_space;
		struct {
			char 	__fp_xmm[512];
		} __fp_xmm_state;
		int 		__fp_fpregs[128];
	} __fp_reg_set;
	long 			__fp_wregs[33]; /* Weitek? */
};
typedef struct fpregset fpregset_t;

struct mcontext {
	gregset_t		mc_gregs;
	fpregset_t		mc_fpregs;

	int				mc_len;			/* sizeof(mcontext_t) */
	__register_t 	mc_flags;
};
typedef struct mcontext mcontext_t;

#define _UC_FXSAVE					0x20	/* FP state is in FXSAVE format in XMM space */
#define _UC_MACHINE_PAD				5	/* Padding appended to ucontext_t */

#define _UC_UCONTEXT_ALIGN			(~0xf)

#ifdef VM86
#define PSL_VM 						0x00020000
#define _UC_MACHINE_SP(uc) ((uc)->uc_mcontext.mc_gregs.mc_esp + \
		((uc)->uc_mcontext.mc_gregs.mc_efl & PSL_VM ? 			\
				((uc)->uc_mcontext.mc_gregs.mc_ss << 4) : 0))
#endif /* VM86 */

#ifndef _UC_MACHINE_SP
#define _UC_MACHINE_SP(uc)			((uc)->uc_mcontext.mc_gregs.mc_esp)
#endif
#define _UC_MACHINE_PC(uc)			((uc)->uc_mcontext.mc_gregs.mc_eip)
#define _UC_MACHINE_INTRV(uc)		((uc)->uc_mcontext.mc_gregs.mc_eax)

#define	_UC_MACHINE_SET_PC(uc, pc)	_UC_MACHINE_PC(uc) = (pc)

#endif /* _I386_INCLUDE_MCONTEXT_H_ */
