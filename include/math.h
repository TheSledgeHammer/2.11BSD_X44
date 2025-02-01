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

#ifdef __HAVE_LONG_DOUBLE
#define	__fpmacro_unary_floating(__name, __arg0)			\
	/* LINTED */							\
	((sizeof (__arg0) == sizeof (float))				\
	?	__ ## __name ## f (__arg0)				\
	: (sizeof (__arg0) == sizeof (double))				\
	?	__ ## __name ## d (__arg0)				\
	:	__ ## __name ## l (__arg0))
#else
#define	__fpmacro_unary_floating(__name, __arg0)			\
	/* LINTED */							\
	((sizeof (__arg0) == sizeof (float))				\
	?	__ ## __name ## f (__arg0)				\
	:	__ ## __name ## d (__arg0))
#endif /* __HAVE_LONG_DOUBLE */

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
/*
 * ANSI/POSIX
 */
double	acos(double);
double	asin(double);
double	atan(double);
double	atan2(double, double);
double	cos(double);
double	sin(double);
double	tan(double);

double	cosh(double);
double	sinh(double);
double	tanh(double);

double	exp(double);
double	frexp(double, int *);
double	ldexp(double, int);
double	log(double);
double	log2(double);
double	log10(double);
double	modf(double, double *);

double	pow(double, double);
double	sqrt(double);

double	ceil(double);
double	fabs(double);
double	floor(double);
double	fmod(double, double);

#if defined(_XOPEN_SOURCE) || defined(__BSD_VISIBLE)
double	erf(double);
double	erfc(double);
double	gamma(double);
double	hypot(double, double);
int	finite(double);
double	j0(double);
double	j1(double);
double	jn(int, double);
double	lgamma(double);
double	y0(double);
double	y1(double);
double	yn(int, double);

#if (_XOPEN_SOURCE - 0) >= 500 || defined(__BSD_VISIBLE)
double	acosh(double);
double	asinh(double);
double	atanh(double);
double	cbrt(double);
double	expm1(double);
int	ilogb(double);
double	log1p(double);
double	logb(double);
double	nextafter(double, double);
double	remainder(double, double);
double	rint(double);
double	scalb(double, double);
#endif /* (_XOPEN_SOURCE - 0) >= 500 || defined(__BSD_VISIBLE)*/
#endif /* _XOPEN_SOURCE || __BSD_VISIBLE */

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(__BSD_VISIBLE)
/* 7.12.3.1 int fpclassify(real-floating x) */
#define	fpclassify(__x)	__fpmacro_unary_floating(fpclassify, __x)

/* 7.12.3.2 int isfinite(real-floating x) */
#define	isfinite(__x)	__fpmacro_unary_floating(isfinite, __x)

/* 7.12.3.5 int isnormal(real-floating x) */
#define	isnormal(__x)	(fpclassify(__x) == FP_NORMAL)

/* 7.12.3.6 int signbit(real-floating x) */
#define	signbit(__x)	__fpmacro_unary_floating(signbit, __x)

/* 7.12.4 trigonometric */

float	acosf(float);
float	asinf(float);
float	atanf(float);
float	atan2f(float, float);
float	cosf(float);
float	sinf(float);
float	tanf(float);

/* 7.12.5 hyperbolic */

float	acoshf(float);
float	asinhf(float);
float	atanhf(float);
float	coshf(float);
float	sinhf(float);
float	tanhf(float);

/* 7.12.6 exp / log */

float	expf(float);
float	expm1f(float);
float	frexpf(float, int *);
int	ilogbf(float);
float	ldexpf(float, int);
float	logf(float);
float	log2f(float);
float	log10f(float);
float	log1pf(float);
float	logbf(float);
float	modff(float, float *);
float	scalbnf(float, int);

/* 7.12.7 power / absolute */

float	cbrtf(float);
float	fabsf(float);
float	hypotf(float, float);
float	powf(float, float);
float	sqrtf(float);

/* 7.12.8 error / gamma */

float	erff(float);
float	erfcf(float);
float	lgammaf(float);

/* 7.12.9 nearest integer */

float	ceilf(float);
float	floorf(float);
float	rintf(float);
double	round(double);
float	roundf(float);
double	trunc(double);
float	truncf(float);
long int	lrint(double);
long int	lrintf(float);
/* LONGLONG */
long long int	llrint(double);
/* LONGLONG */
long long int	llrintf(float);
long int	lround(double);
long int	lroundf(float);
/* LONGLONG */
long long int	llround(double);
/* LONGLONG */
long long int	llroundf(float);

/* 7.12.10 remainder */

float	fmodf(float, float);
float	remainderf(float, float);

/* 7.12.11 manipulation */

float	copysignf(float, float);
double	nan(const char *);
float	nanf(const char *);
long double	nanl(const char *);
float	nextafterf(float, float);

/* 7.12.14 comparision */

#define isunordered(x, y)	(isnan(x) || isnan(y))
#define isgreater(x, y)		(!isunordered((x), (y)) && (x) > (y))
#define isgreaterequal(x, y)	(!isunordered((x), (y)) && (x) >= (y))
#define isless(x, y)		(!isunordered((x), (y)) && (x) < (y))
#define islessequal(x, y)	(!isunordered((x), (y)) && (x) <= (y))
#define islessgreater(x, y)	(!isunordered((x), (y)) && \
				 ((x) > (y) || (y) > (x)))
double	fdim(double, double);
double	fmax(double, double);
double	fmin(double, double);
float	fdimf(float, float);
float	fmaxf(float, float);
float	fminf(float, float);
long double fdiml(long double, long double);
long double fmaxl(long double, long double);
long double fminl(long double, long double);

#endif /* !_ANSI_SOURCE && ... */

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) || \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    defined(_ISOC99_SOURCE) || defined(__BSD_VISIBLE)
/* 7.12.3.3 int isinf(real-floating x) */
#ifdef __isinf
#define	isinf(__x)	__isinf(__x)
#else
#define	isinf(__x)	__fpmacro_unary_floating(isinf, __x)
#endif

/* 7.12.3.4 int isnan(real-floating x) */
#ifdef __isnan
#define	isnan(__x)	__isnan(__x)
#else
#define	isnan(__x)	__fpmacro_unary_floating(isnan, __x)
#endif
#endif /* !_ANSI_SOURCE && ... */

#if defined(__BSD_VISIBLE)
#ifndef __cplusplus
int	matherr(struct exception *);
#endif

/*
 * IEEE Test Vector
 */
double	significand(double);

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
double	copysign(double, double);
double	scalbn(double, int);

/*
 * BSD math library entry points
 */
double	drem(double, double);

#endif /* __BSD_VISIBLE */

#if defined(__BSD_VISIBLE) || defined(_REENTRANT)
/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
double	gamma_r(double, int *);
double	lgamma_r(double, int *);
#endif /* __BSD_VISIBLE || _REENTRANT */

#if defined(__BSD_VISIBLE)

/* float versions of ANSI/POSIX functions */

float	gammaf(float);
int	isinff(float);
int	isnanf(float);
int	finitef(float);
float	j0f(float);
float	j1f(float);
float	jnf(int, float);
float	y0f(float);
float	y1f(float);
float	ynf(int, float);

float	scalbf(float, float);

/*
 * float version of IEEE Test Vector
 */
float	significandf(float);

/*
 * float versions of BSD math library entry points
 */
float	dremf(float, float);
#endif /* __BSD_VISIBLE */

#if defined(__BSD_VISIBLE) || defined(_REENTRANT)
/*
 * Float versions of reentrant version of gamma & lgamma; passes
 * signgam back by reference as the second argument; user must
 * allocate space for signgam.
 */
float	gammaf_r(float, int *);
float	lgammaf_r(float, int *);
#endif /* !... || _REENTRANT */

/*
 * Library implementation
 */
int	__fpclassifyf(float);
int	__fpclassifyd(double);
int	__isfinitef(float);
int	__isfinited(double);
int	__isinff(float);
int	__isinfd(double);
int	__isnanf(float);
int	__isnand(double);
int	__signbitf(float);
int	__signbitd(double);

#ifdef __HAVE_LONG_DOUBLE
int	__fpclassifyl(long double);
int	__isfinitel(long double);
int	__isinfl(long double);
int	__isnanl(long double);
int	__signbitl(long double);
#endif
__END_DECLS

#endif /* _MATH_H_ */
