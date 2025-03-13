/*	$NetBSD: float_ieee754.h,v 1.11 2013/06/18 20:17:19 christos Exp $	*/

/*
 * Copyright (c) 1992, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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

#ifndef _FLOAT_H_
#define _FLOAT_H_

#include <sys/cdefs.h>

#if !defined(__ASSEMBLER__) && !defined(FLT_ROUNDS)
__BEGIN_DECLS
extern int __flt_rounds(void);
__END_DECLS
#define FLT_ROUNDS	__flt_rounds()
#endif

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(__BSD_VISIBLE)
#ifndef FLT_EVAL_METHOD
#if __GNUC_PREREQ__(3, 3)
#define FLT_EVAL_METHOD	__FLT_EVAL_METHOD__
#endif /* GCC >= 3.3 */
#endif /* defined(FLT_EVAL_METHOD) */
#endif /* !defined(_ANSI_SOURCE) && ... */

#if __GNUC_PREREQ__(3, 3)

/*
 * GCC 3,3 and later provide builtins for the FLT, DBL, and LDBL constants.
 */
#define FLT_RADIX	__FLT_RADIX__

#define FLT_MANT_DIG	__FLT_MANT_DIG__
#define FLT_EPSILON	__FLT_EPSILON__
#define FLT_DIG		__FLT_DIG__
#define FLT_MIN_EXP	__FLT_MIN_EXP__
#define FLT_MIN		__FLT_MIN__
#define FLT_MIN_10_EXP	__FLT_MIN_10_EXP__
#define FLT_MAX_EXP	__FLT_MAX_EXP__
#define FLT_MAX		__FLT_MAX__
#define FLT_MAX_10_EXP	__FLT_MAX_10_EXP__

#define DBL_MANT_DIG	__DBL_MANT_DIG__
#define DBL_EPSILON	__DBL_EPSILON__
#define DBL_DIG		__DBL_DIG__
#define DBL_MIN_EXP	__DBL_MIN_EXP__
#define DBL_MIN		__DBL_MIN__
#define DBL_MIN_10_EXP	__DBL_MIN_10_EXP__
#define DBL_MAX_EXP	__DBL_MAX_EXP__
#define DBL_MAX		__DBL_MAX__
#define DBL_MAX_10_EXP	__DBL_MAX_10_EXP__

#else /* GCC < 3.3 */

#include <machine/float.h>

#define FLT_RADIX	__FLT_RADIX			/* b */

#define FLT_ROUNDS	__FLT_ROUNDS	  		/* FP addition rounds to nearest (1) */
#define FLT_EVAL_METHOD	__FLT_EVAL_METHOD		/* long double */

#define FLT_MANT_DIG	__FLT_MANT_DIG			/* p */
#define FLT_EPSILON	__FLT_EPSILON			/* b**(1-p) */
#define FLT_DIG		__FLT_DIG			/* floor((p-1)*log10(b))+(b == 10) */
#define FLT_MIN_EXP	__FLT_MIN_EXP			/* emin */
#define FLT_MIN		__FLT_MIN			/* b**(emin-1) */
#define FLT_MIN_10_EXP	__FLT_MIN_10_EXP		/* ceil(log10(b**(emin-1))) */
#define FLT_MAX_EXP	__FLT_MAX_EXP			/* emax */
#define FLT_MAX		__FLT_MAX			/* (1-b**(-p))*b**emax */
#define FLT_MAX_10_EXP	__FLT_MAX_10_EXP		/* floor(log10((1-b**(-p))*b**emax)) */

#define DBL_MANT_DIG	__DBL_MANT_DIG
#define DBL_EPSILON	__DBL_EPSILON
#define DBL_DIG		__DBL_DIG
#define DBL_MIN_EXP	__DBL_MIN_EXP
#define DBL_MIN		__DBL_MIN
#define DBL_MIN_10_EXP	__DBL_MIN_10_EXP
#define DBL_MAX_EXP	__DBL_MAX_EXP
#define DBL_MAX		__DBL_MAX
#define DBL_MAX_10_EXP	__DBL_MAX_10_EXP

#define LDBL_MANT_DIG	__LDBL_MANT_DIG
#define LDBL_EPSILON	__LDBL_EPSILON
#define LDBL_DIG	__LDBL_DIG
#define LDBL_MIN_EXP	__LDBL_MIN_EXP
#define LDBL_MIN	__LDBL_MIN
#define LDBL_MIN_10_EXP	__LDBL_MIN_10_EXP
#define LDBL_MAX_EXP	__LDBL_MAX_EXP
#define LDBL_MAX	__LDBL_MAX
#define LDBL_MAX_10_EXP	__LDBL_MAX_10_EXP

#define	DECIMAL_DIG	__DECIMAL_DIG

#endif /* GCC < 3.3 */

#endif /* _FLOAT_H_ */
