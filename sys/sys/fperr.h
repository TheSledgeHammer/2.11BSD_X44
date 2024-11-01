/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)fperr.h	1.2 (2.11BSD) 1997/8/28
 */

#ifndef SYS_FPERR_H_
#define SYS_FPERR_H_

#include <sys/types.h>

/*
 * Structure of the floating point error register save/return
 */
struct fperr {
	int		f_fec;
	caddr_t	f_fea;
};

/* SIGFPE (Floating Point Error) */
#define	FPE_OPCODE_TRAP		0x01		/* Bad FP opcode */
#define	FPE_FLTDIV_TRAP 	0x02		/* FP divide by zero */
#define	FPE_INTOVF_TRAP		0x03		/* FP to INT overflow */
#define	FPE_FLTOVF_TRAP		0x04		/* FP overflow */
#define	FPE_FLTUND_TRAP		0x05		/* FP underflow */
#define	FPE_UNDEF_TRAP		0x06		/* FP Undefined variable */
#define	FPE_MAINT_TRAP		0x07		/* FP Maint trap */
#define	FPE_FPU_NP_TRAP		0x08		/* FP unit not present */
#define	FPE_SUBRNG_TRAP		0x09		/* subrange out of bounds */
#define	FPE_INTDIV_TRAP		0x0A		/* integer divide by zero */
#define	FPE_DECOVF_TRAP		0x0B		/* decimal overflow */
#define	FPE_FLTOVF_FAULT	0x0C		/* floating overflow fault */
#define	FPE_FLTDIV_FAULT	0x0D		/* divide by zero floating fault */
#define	FPE_FLTUND_FAULT	0x0E		/* floating underflow fault */

#define FPE_CRAZY		0x0F		/* illegal return code - FPU crazy */
#define FPE_OPERAND_TRAP	0x10		/* bad floating point operand */

/* SIGILL */
#define	ILL_ILLOPC_TRAP		0x01		/* Illegal opcode */
#define	ILL_ILLOPN_TRAP		0x02		/* Illegal operand */
#define	ILL_ILLADR_TRAP		0x03		/* Illegal addressing mode */
#define	ILL_ILLTRP_TRAP		0x04		/* Illegal trap	*/
#define	ILL_PRVOPC_TRAP		0x05		/* Privileged opcode */
#define	ILL_PRVREG_TRAP		0x06		/* Privileged register */
#define	ILL_COPROC_TRAP		0x07		/* Coprocessor error */
#define	ILL_BADSTK_TRAP		0x08		/* Internal stack error	*/

/* SIGTRAP */
#define	TRAP_BRKPT_TRAP		0x01		/* Process breakpoint */
#define	TRAP_TRACE_TRAP		0x02		/* Process trace trap */
#define	TRAP_EXEC_TRAP		0x03		/* Process exec trap */
#define	TRAP_CHLD_TRAP		0x04		/* Process child trap */
#define	TRAP_PROC_TRAP		0x05		/* Process trap */
#define	TRAP_DBREG_TRAP		0x06		/* Process hardware debug register trap	*/
#define	TRAP_SCE_TRAP		0x07		/* Process syscall entry trap */
#define	TRAP_SCX_TRAP		0x08		/* Process syscall exit trap */

#endif /* SYS_FPERR_H_ */
