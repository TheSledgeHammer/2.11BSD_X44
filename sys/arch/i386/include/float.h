/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)float.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _I386_FLOAT_H_
#define	_I386_FLOAT_H_

static const int map[] = {
		1,	/* round to nearest */
		3,	/* round to zero */
		2,	/* round to negative infinity */
		0	/* round to positive infinity */
};

static __inline int
__flt_rounds(void)
{
	int x;

	__asm("fnstcw %0" : "=m" (x));
	return (map[(x >> 10) & 0x03]);
}

#ifndef _KERNEL

#define __FLT_RADIX		2				/* b */
#define __FLT_ROUNDS		__flt_rounds()  		/* FP addition rounds to nearest (1) */
#define __FLT_EVAL_METHOD	2				/* long double */

#define __FLT_MANT_DIG		24				/* p */
#define __FLT_EPSILON		1.19209290E-07F			/* b**(1-p) */
#define __FLT_DIG		6				/* floor((p-1)*log10(b))+(b == 10) */
#define __FLT_MIN_EXP		(-125)				/* emin */
#define __FLT_MIN		1.17549435E-38F			/* b**(emin-1) */
#define __FLT_MIN_10_EXP	(-37)				/* ceil(log10(b**(emin-1))) */
#define __FLT_MAX_EXP		128				/* emax */
#define __FLT_MAX		3.40282347E+38F			/* (1-b**(-p))*b**emax */
#define __FLT_MAX_10_EXP	38				/* floor(log10((1-b**(-p))*b**emax)) */

#define __DBL_MANT_DIG		53
#define __DBL_EPSILON		2.2204460492503131E-16
#define __DBL_DIG		15
#define __DBL_MIN_EXP		(-1021)
#define __DBL_MIN		2.225073858507201E-308
#define __DBL_MIN_10_EXP	(-307)
#define __DBL_MAX_EXP		1024
#define __DBL_MAX		1.797693134862316E+308
#define __DBL_MAX_10_EXP	308

#define __LDBL_MANT_DIG		64
#define __LDBL_EPSILON		1.0842021724855044340E-19L
#define __LDBL_DIG		18
#define __LDBL_MIN_EXP		(-16381)
#define __LDBL_MIN		3.3621031431120935063E-4932L
#define __LDBL_MIN_10_EXP	(-4931)
#define __LDBL_MAX_EXP		16384
#define __LDBL_MAX		1.1897314953572317650E+4932L
#define __LDBL_MAX_10_EXP	4932

#define	__DECIMAL_DIG		21

#else /* _KERNEL */

#define FLT_RADIX		__FLT_RADIX			/* b */
#define FLT_ROUNDS		__FLT_ROUNDS  			/* FP addition rounds to nearest (1) */
#define FLT_EVAL_METHOD		__FLT_EVAL_METHOD		/* long double */

#define FLT_MANT_DIG		__FLT_MANT_DIG			/* p */
#define FLT_EPSILON		__FLT_EPSILON			/* b**(1-p) */
#define FLT_DIG			__FLT_DIG			/* floor((p-1)*log10(b))+(b == 10) */
#define FLT_MIN_EXP		__FLT_MIN_EXP			/* emin */
#define FLT_MIN			__FLT_MIN			/* b**(emin-1) */
#define FLT_MIN_10_EXP		__FLT_MIN_10_EXP		/* ceil(log10(b**(emin-1))) */
#define FLT_MAX_EXP		__FLT_MAX_EXP			/* emax */
#define FLT_MAX			__FLT_MAX			/* (1-b**(-p))*b**emax */
#define FLT_MAX_10_EXP		__FLT_MAX_10_EX			/* floor(log10((1-b**(-p))*b**emax)) */

#define DBL_MANT_DIG		__DBL_MANT_DIG
#define DBL_EPSILON		__DBL_EPSILON
#define DBL_DIG			__DBL_DIG
#define DBL_MIN_EXP		__DBL_MIN_EXP
#define DBL_MIN			__DBL_MIN
#define DBL_MIN_10_EXP		__DBL_MIN_10_EXP
#define DBL_MAX_EXP		__DBL_MAX_EXP
#define DBL_MAX			__DBL_MAX
#define DBL_MAX_10_EXP		__DBL_MAX_10_EXP

#define LDBL_MANT_DIG		__LDBL_MANT_DIG
#define LDBL_EPSILON		__LDBL_EPSILON
#define LDBL_DIG		__LDBL_DIG
#define LDBL_MIN_EXP		__LDBL_MIN_EXP
#define LDBL_MIN		__LDBL_MIN
#define LDBL_MIN_10_EXP		__LDBL_MIN_10_EXP
#define LDBL_MAX_EXP		__LDBL_MAX_EXP
#define LDBL_MAX		__LDBL_MAX
#define LDBL_MAX_10_EXP		__LDBL_MAX_10_EXP

#define	DECIMAL_DIG		__DECIMAL_DIG

#endif /* _KERNEL */

#endif /* !_I386_FLOAT_H_ */
