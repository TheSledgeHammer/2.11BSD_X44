/*
 * Copyright (c) 1985, 1990, 1993
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
 *	@(#)math.h	8.1 (Berkeley) 6/2/93
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)math.h	1.1 (2.10BSD Berkeley) 12/1/86
 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 */

#ifndef _MATH_H_
#define _MATH_H_

#include <sys/cdefs.h>

union __float_u {
	unsigned char __dummy[sizeof(float)];
	float __val;
};

union __double_u {
	unsigned char __dummy[sizeof(double)];
	double __val;
};

union __long_double_u {
	unsigned char __dummy[sizeof(long double)];
	long double __val;
};

/*
 * ANSI/POSIX
 */
/* 7.12#3 HUGE_VAL, HUGELF, HUGE_VALL */
#if __GNUC_PREREQ__(3, 3)
#define HUGE_VAL	__builtin_huge_val()
#else
extern __const union __double_u __infinity;
#define HUGE_VAL	__infinity.__val
#endif

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(__BSD_VISIBLE)

/* 7.12#3 HUGE_VAL, HUGELF, HUGE_VALL */
#if __GNUC_PREREQ__(3, 3)
#define	HUGE_VALF	__builtin_huge_valf()
#define	HUGE_VALL	__builtin_huge_vall()
#define	INFINITY	__builtin_inff()
#define	NAN		__builtin_nanf("")
#else /* __GNUC_PREREQ__(3, 3) */
extern __const union __float_u __infinityf;
#define	HUGE_VALF	__infinityf.__val
#define	INFINITY	HUGE_VALF

extern __const union __long_double_u __infinityl;
#define	HUGE_VALL	__infinityl.__val

extern __const union __float_u __nanf;
#define	NAN		 __nanf.__val
#endif /* __GNUC_PREREQ__(3, 3) */

#endif /* !_ANSI_SOURCE && ... */

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE) \
    || defined(_XOPEN_SOURCE) || defined(__BSD_VISIBLE)

#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

#define	MAXFLOAT	((float)3.40282346638528860e+38)
extern int signgam;

#if defined(vax) || defined(tahoe) || defined(pdp11)
#define	HUGE	    1.701411733192644270E+38
#else
#define	HUGE	    MAXFLOAT
#endif

#define	LOGHUGE	    39
#endif /* !_ANSI_SOURCE && !_POSIX_SOURCE */

__BEGIN_DECLS

double	fabs(double), floor(double), ceil(double), fmod(double, double), ldexp(double, int);
double	sqrt(double), hypot(double, double), atof(const char *);
double	sin(double), cos(double), tan(double), asin(double), acos(double), atan(double), atan2(double, double);
double	exp(double), log(double), log10(double), pow(double, double);
double	sinh(double), cosh(double), tanh(double);
double	gamma(double);
double	j0(double), j1(double), jn(int, double), y0(double), y1(double), yn(int, double);

/* libc/gen/arch/"arch" */
double frexp(double, int *);
int    isnan(double);
int    isinf(double);
__END_DECLS

#endif /* _MATH_H_ */
