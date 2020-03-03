/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)fperr.h	1.2 (2.11BSD) 1997/8/28
 */

#ifndef SYS_FPERR_H_
#define SYS_FPERR_H_

/*
 * Structure of the floating point error register save/return
 */
struct fperr {
	int		f_fec;
	caddr_t	f_fea;
};

/* SIGFPE (Floating Point Error) */
#define	FPE_OPCODE_TRAP	0x2		/* Bad FP opcode */
#define	FPE_FLTDIV_TRAP 0x4		/* FP divide by zero */
#define	FPE_INTOVF_TRAP	0x6		/* FP to INT overflow */
#define	FPE_FLTOVF_TRAP	0x8		/* FP overflow */
#define	FPE_FLTUND_TRAP	0x10	/* FP underflow */
#define	FPE_UNDEF_TRAP	0x12	/* FP Undefined variable */
#define	FPE_MAINT_TRAP	0x14	/* FP Maint trap */

/* SIGILL */
#define	ILL_ILLOPC_TRAP	0x2		/* Illegal opcode */
#define	ILL_ILLOPN_TRAP	0x4		/* Illegal operand */
#define	ILL_ILLADR_TRAP	0x6		/* Illegal addressing mode */
#define	ILL_ILLTRP_TRAP	0x8		/* Illegal trap	*/
#define	ILL_PRVOPC_TRAP	0x10	/* Privileged opcode */
#define	ILL_PRVREG_TRAP	0x12	/* Privileged register */
#define	ILL_COPROC_TRAP	0x14	/* Coprocessor error */
#define	ILL_BADSTK_TRAP	0x16	/* Internal stack error	*/

/* SIGTRAP */
#define	TRAP_BRKPT_TRAP	0x2		/* Process breakpoint */
#define	TRAP_TRACE_TRAP	0x4		/* Process trace trap */
#define	TRAP_EXEC_TRAP	0x6		/* Process exec trap */
#define	TRAP_CHLD_TRAP	0x8		/* Process child trap */
#define	TRAP_PROC_TRAP	0x10	/* Process trap */
#define	TRAP_DBREG_TRAP	0x12	/* Process hardware debug register trap	*/
#define	TRAP_SCE_TRAP	0x14	/* Process syscall entry trap */
#define	TRAP_SCX_TRAP	0x16	/* Process syscall exit trap */

#endif /* SYS_FPERR_H_ */
