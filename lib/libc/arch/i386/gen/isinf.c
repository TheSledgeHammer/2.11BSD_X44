/*-
 * Copyright (c) 1991, 1993
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)isinf.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/ieee.h>
#include <math.h>

#ifdef __weak_alias
__weak_alias(isnan,__isnan)
__weak_alias(isinf,__isinf)
#endif

/* IEEE 754 Double Precision */

/*
 * 7.12.3.4 isnan - test for a NaN
 *          IEEE 754 double-precision version
 */
int
__isnan(d)
	double d;
{
	register struct ieee_double *p = (struct ieee_double *)&d;

    return (p->dbl_exp == DBL_EXP_INFNAN && (p->dbl_frach || p->dbl_fracl));
}

int
__isnand(d)
	double d;
{
	return (__isnan(d));
}

/*
 * 7.12.3.3 isinf - test for infinity
 *          IEEE 754 double-precision version
 */
int
__isinf(d)
	double d;
{
	register struct ieee_double *p = (struct ieee_double *)&d;
	
	return (p->dbl_exp == DBL_EXP_INFNAN && !p->dbl_frach && !p->dbl_fracl);
}

int
__isinfd(d)
	double d;
{
	return (__isinf(d));
}

/*
 * 7.12.3.1 fpclassify - classify real floating type
 *          IEEE 754 double-precision version
 */
int
__fpclassify(d)
	double d;
{
	register struct ieee_double *p = (struct ieee_double *)&d;

	if (p->dbl_exp == 0) {
		if (!p->dbl_frach && !p->dbl_fracl) {
			return (FP_ZERO);
		} else {
			return (FP_SUBNORMAL);
		}
	} else if (p->dbl_exp == DBL_EXP_INFNAN) {
		if (!p->dbl_frach && !p->dbl_fracl) {
			return (FP_INFINITE);
		} else {
			return (FP_NAN);
		}
	}
	return (FP_NORMAL);
}

int
__fpclassifyd(d)
	double d;
{
	return (__fpclassify(d));
}

/*
 * 7.12.3.2 isfinite - determine whether an argument has finite value
 *          IEEE 754 double-precision version
 */
int
__isfinite(d)
	double d;
{
	register struct ieee_double *p = (struct ieee_double *)&d;

	if (p->dbl_exp == DBL_EXP_INFNAN) {
		return (0);
	}
	return (1);
}

int
__isfinited(d)
	double d;
{
	return (__isfinite(d));
}


/*
 * 7.12.3.6 signbit - determine whether the sign of an argument is negative
 *          IEEE 754 double-precision version
 */
int
__signbit(d)
	double d;
{
	register struct ieee_double *p = (struct ieee_double *)&d;

	return (p->dbl_sign == 1);
}

int
__signbitd(d)
	double d;
{
	return (__signbit(d));
}

/* IEEE 754 Extended Precision */

/*
 * 7.12.3.4 isnan - test for a NaN
 *          IEEE 754 compatible 80-bit extended-precision Intel 386 version
 */
int
__isnanl(d)
	long double d;
{
	register struct ieee_ext *p = (struct ieee_ext *)&d;
	
	p->ext_frach &= ~0x80000000;	/* clear normalization bit */
	
	return (p->ext_exp == EXT_EXP_INFNAN && (p->ext_frach || p->ext_fracl));
}

/*
 * 7.12.3.3 isinf - test for infinity
 *          IEEE 754 compatible 80-bit extended-precision Intel 386 version
 */
int
__isinfl(d)
    	long double d;
{
	register struct ieee_ext *p = (struct ieee_ext *)&d;
	
	p->ext_frach &= ~0x80000000;	/* clear normalization bit */
	
	return (p->ext_exp == EXT_EXP_INFNAN && !p->ext_frach && !p->ext_fracl);
}

/*
 * 7.12.3.1 fpclassify - classify real floating type
 *          IEEE 754 compatible 128-bit extended-precision version
 */
int
__fpclassifyl(d)
	long double d;
{
	register struct ieee_ext *p = (struct ieee_ext *)&d;

	if (p->ext_exp == 0) {
		if (!p->dbl_frach && !p->dbl_fracl) {
			return (FP_ZERO);
		} else {
			return (FP_SUBNORMAL);
		}
	} else if (p->ext_exp == EXT_EXP_INFNAN) {
		if (!p->dbl_frach && !p->dbl_fracl) {
			return (FP_INFINITE);
		} else {
			return (FP_NAN);
		}
	}
	return (FP_NORMAL);
}

/*
 * 7.12.3.2 isfinite - determine whether an argument has finite value
 *          IEEE 754 compatible 128-bit extended-precision version
 */
int
__isfinitel(d)
	long double d;
{
	register struct ieee_ext *p = (struct ieee_ext *)&d;

	if (p->ext_exp == EXT_EXP_INFNAN) {
		return (0);
	}

	return (1);
}

/*
 * 7.12.3.6 signbit - determine whether the sign of an argument is negative
 *          IEEE 754 compatible 80-bit or 128-bit extended precision.
 *
 *          Differences between 128-bit and 80-bit are handled by having a
 *          different struct ieee_ext.
 */
int
__signbitl(d)
	double d;
{
	register struct ieee_ext *p = (struct ieee_ext *)&d;

	return (p->ext_sign == 1);
}

/* IEEE 754 Single Precision */

/*
 * 7.12.3.4 isnan - test for a NaN
 *          IEEE 754 single-precision version
 */
int
__isnanf(f)
    	float f;
{
	register struct ieee_single *p = (struct ieee_single *)&f;

	return (p->sng_exp == SNG_EXP_INFNAN && p->sng_frac);
}

/*
 * 7.12.3.3 isinf - test for infinity
 *          IEEE 754 single-precision version
 */
int
__isinff(f)
    	float f;
{
	register struct ieee_single *p = (struct ieee_single *)&f;

	return (p->sng_exp == SNG_EXP_INFNAN && !p->sng_frac);
}

/*
 * 7.12.3.1 fpclassify - classify real floating type
 *          IEEE 754 single-precision version
 */
int
__fpclassifyf(f)
	float f;
{
	register struct ieee_single *p = (struct ieee_single *)&f;

	if (p->sng_exp == 0) {
		if (!p->sng_frac) {
			return (FP_ZERO);
		} else {
			return (FP_SUBNORMAL);
		}
	} else if (p->sng_exp == SNG_EXP_INFNAN) {
		if (!p->sng_frac) {
			return (FP_INFINITE);
		} else {
			return (FP_NAN);
		}
	}
	return (FP_NORMAL);
}

/*
 * 7.12.3.2 isfinite - determine whether an argument has finite value
 *          IEEE 754 single-precision version
 */
int
__isfinitef(f)
	float f;
{
	register struct ieee_single *p = (struct ieee_single *)&f;

	if (p->sng_exp == SNG_EXP_INFNAN) {
		return (0);
	}
	return (1);
}

/*
 * 7.12.3.6 signbit - determine whether the sign of an argument is negative
 *          IEEE 754 single-precision version
 */
int
__signbitf(f)
	float f;
{
	register struct ieee_single *p = (struct ieee_single *)&f;

	return (p->sng_sign == 1);
}
