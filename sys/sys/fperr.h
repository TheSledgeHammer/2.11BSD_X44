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

#define	FPE_OPCODE_TRAP	0x2	/* Bad FP opcode */
#define	FPE_FLTDIV_TRAP 0x4	/* FP divide by zero */
#define	FPE_INTOVF_TRAP	0x6	/* FP to INT overflow */
#define	FPE_FLTOVF_TRAP	0x8	/* FP overflow */
#define	FPE_FLTUND_TRAP	0xa	/* FP underflow */
#define	FPE_UNDEF_TRAP	0xc	/* FP Undefined variable */
#define	FPE_MAINT_TRAP	0xe	/* FP Maint trap */

#endif /* SYS_FPERR_H_ */
