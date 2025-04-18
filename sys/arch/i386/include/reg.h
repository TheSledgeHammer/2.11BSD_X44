/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)reg.h	8.1 (Berkeley) 6/11/93
 */

#ifndef	_I386_REG_H_
#define	_I386_REG_H_

#include <machine/types.h>
#include <machine/npx.h>

/*
 * Location of the users' stored
 * registers within appropriate frame of 'trap' and 'syscall', relative to
 * base of stack frame.
 * Normal usage is u.u_ar0[XX] in kernel.
 */

/* When referenced during a trap/exception, registers are at these offsets */

#define	tES	(0)
#define	tDS	(1)
#define	tEDI	(2)
#define	tESI	(3)
#define	tEBP	(4)

#define	tEBX	(6)
#define	tEDX	(7)
#define	tECX	(8)
#define	tEAX	(9)

#define	tEIP	(12)
#define	tCS	(13)
#define	tEFLAGS	(14)
#define	tESP	(15)
#define	tSS	(16)

/* During a system call, registers are at these offsets instead of above. */

#define	sEDI	(0)
#define	sESI	(1)
#define	sEBP	(2)

#define	sEBX	(4)
#define	sEDX	(5)
#define	sECX	(6)
#define	sEAX	(7)
#define	sEFLAGS	(8)
#define	sEIP	(9)
#define	sCS	(10)
#define	sESP	(11)
#define	sSS	(12)

#define	PC	sEIP
#define	SP	sESP
#define	PS	sEFLAGS
#define	R0	sEDX
#define	R1	sECX

/*
 * Registers accessible to ptrace(2) syscall for debugger
 */
#ifdef IPCREG
#define	NIPCREG 14
int ipcreg[NIPCREG] = {
		tES,
		tDS,
		tEDI,
		tESI,
		tEBP,
		tEBX,
		tEDX,
		tECX,
		tEAX,
		tEIP,
		tCS,
		tEFLAGS,
		tESP,
		tSS
};
#endif

/*
 * Register set accessible via /proc/$pid/regs and PT_{SET,GET}REGS.
 */
struct reg32 {
	u_int32_t	r_fs;
	u_int32_t	r_es;
	u_int32_t	r_ds;
	u_int32_t	r_edi;
	u_int32_t	r_esi;
	u_int32_t	r_ebp;
	u_int32_t	r_isp;
	u_int32_t	r_ebx;
	u_int32_t	r_edx;
	u_int32_t	r_ecx;
	u_int32_t	r_eax;
	u_int32_t	r_trapno;
	u_int32_t	r_err;
	u_int32_t	r_eip;
	u_int32_t	r_cs;
	u_int32_t	r_eflags;
	u_int32_t	r_esp;
	u_int32_t	r_ss;
	u_int32_t	r_gs;
};

struct fpreg {
	struct save87 fstate;
};

struct xmmregs {
	struct fxsave fxstate;
};

/*
 * Debug Registers
 *
 * DR0-DR3  Debug Address Registers
 * DR4-DR5  Reserved
 * DR6      Debug Status Register
 * DR7      Debug Control Register
 */
struct dbreg {
	int	dr[8];
};

#ifdef _KERNEL
struct proc;
#endif

#endif	/* _I386_REG_H_ */
