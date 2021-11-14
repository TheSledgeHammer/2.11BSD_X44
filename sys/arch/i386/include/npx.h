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
 *	@(#)npx.h	8.1 (Berkeley) 6/11/93
 */

#ifndef	_I386_NPX_H_
#define	_I386_NPX_H_

/*
 * 287/387 NPX Coprocessor Data Structures and Constants
 * W. Jolitz 1/90
 */

/* Default x87 control word. */
#define	___NPX87___		0x037f

/* Default values for the mxcsr. All traps masked. */
#define ___MXCSR___		0x1f80
#define	__MXCSR_MASK__	0xffbf

/* Environment information of floating point unit */
struct	env87 {
	long	en_cw;							/* control word (16bits) */
	long	en_sw;							/* status word (16bits) */
	long	en_tw;							/* tag word (16bits) */
	long	en_fip;							/* floating point instruction pointer */
	u_short	en_fcs;							/* floating code segment selector */
	u_short	en_opcode;						/* opcode last executed (11 bits ) */
	long	en_foo;							/* floating operand offset */
	long	en_fos;							/* floating operand segment selector */
};

/* Contents of each floating point accumulator */
struct	fpacc87 {
#ifdef dontdef								/* too unportable */
	u_long	fp_mantlo;						/* mantissa low (31:0) */
	u_long	fp_manthi;						/* mantissa high (63:32) */
	int		fp_exp:15;						/* exponent */
	int		fp_sgn:1;						/* mantissa sign */
#else
	u_char	fp_bytes[10];
#endif
};

/* Floating point context */
struct	save87 {
	struct	env87 	sv_env;					/* floating point control/status */
	struct	fpacc87	sv_ac[8];				/* accumulator contents, 0-7 */
#ifndef dontdef
	u_long			sv_ex_sw;				/* status word for last exception (was pad) */
	u_long			sv_ex_tw;				/* tag word for last exception (was pad) */
	u_char			sv_pad[8 * 2 - 2 * 4];	/* bogus historical padding */
#endif
};

/* Environment information of FPU/MMX/SSE/SSE2 */
struct 	envfx87 { 							/* ENVXMM */
	long				fx_cw;				/* control word (16bits) */
	long				fx_sw;				/* status word (16bits) */
	long				fx_tw;				/* tag word (16bits) */
	long				fx_zero;			/* zero  */
	long				fx_fip;				/* floating point instruction pointer */
	u_short				fx_fcs;				/* floating code segment selector */
	u_short				fx_opcode;			/* opcode last executed (11 bits ) */
	long				fx_foo;				/* floating operand offset */
	long				fx_fos;				/* floating operand segment selector */
	long 				fx_mxcsr;			/* MXCSR Register State */
	long 				fx_mxcsr_mask;		/* Mask for valid MXCSR bit (may be 0) */
};

/* FPU regsters in the extended save format. */
struct 	fpaccfx87 {  						/* FPACCXMM  */
	uint8_t 			fp_bytes[10];
	uint8_t 			fp_rsvd[6];
};

/* SSE/SSE2 registers. */
struct xmmreg {
	uint8_t 			sse_bytes[16];
};

/* FPU/MMX/SSE/SSE2 context */
struct 	fxsave {		/* SAVEXMM  */
	struct envfx87 		fxv_env;			/* fxsave control/status */
	struct fpaccfx87 	fxv_ac[8]; 			/* accumulator contents, 0-7 */
	struct xmmreg 		fxv_xmmregs[8];		/* XMM regs */
	uint8_t 			fxv_rsvd[16 * 14];
	/* 512-bytes --- end of hardware portion of save area */
	uint32_t 			fxv_ex_sw;			/* saved SW from last exception */
	uint32_t 			fxv_ex_tw;			/* saved TW from last exception */
};

struct savefpu {
	struct save87 		sv_87;				/* save87 sub structs: env87, fpacc87 */
	struct fxsave 		sv_fx;				/* fxsave sub structs: envfx87, fpaccfx87 */
};

/* Cyrix EMC memory - mapped coprocessor context switch information */
struct	emcsts {
	long				em_msw;				/* memory mapped status register when swtched */
	long				em_tar;				/* memory mapped temp A register when swtched */
	long				em_dl;				/* memory mapped D low register when swtched */
};

struct proc *npxproc(void);

#endif /* _I386_NPX_H_ */
