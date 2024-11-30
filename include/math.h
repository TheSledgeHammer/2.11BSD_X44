/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)math.h	1.1 (2.10BSD Berkeley) 12/1/86
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
#define	__fpmacro_unary_floating(__name, __arg0)	\
	/* LINTED */							        \
	((sizeof (__arg0) == sizeof (float))			\
	?	__ ## __name ## f (__arg0)				    \
	: (sizeof (__arg0) == sizeof (double))			\
	?	__ ## __name ## d (__arg0)				    \
	:	__ ## __name ## l (__arg0))
#else
#define	__fpmacro_unary_floating(__name, __arg0)	\
	/* LINTED */							        \
	((sizeof (__arg0) == sizeof (float))			\
	?	__ ## __name ## f (__arg0)				    \
	:	__ ## __name ## d (__arg0))
#endif /* __HAVE_LONG_DOUBLE */

/*
 * ANSI/POSIX
 */
/* 7.12#3 HUGE_VAL, HUGELF, HUGE_VALL */
extern __const union __double_u __infinity;
#define HUGE_VAL	__infinity.__val

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
extern __const union __float_u __infinityf;
#define	HUGE_VALF	__infinityf.__val

extern __const union __long_double_u __infinityl;
#define	HUGE_VALL	__infinityl.__val

/* 7.12#4 INFINITY */
#ifdef __INFINITY
#define	INFINITY	__INFINITY	/* float constant which overflows */
#else
#define	INFINITY	HUGE_VALF	/* positive infinity */
#endif /* __INFINITY */

/* 7.12#5 NAN: a quiet NaN, if supported */
#ifdef __HAVE_NANF
extern __const union __float_u __nanf;
#define	NAN		__nanf.__val
#endif /* __HAVE_NANF */

/* 7.12#6 number classification macros */
#define	FP_INFINITE	0x00
#define	FP_NAN		0x01
#define	FP_NORMAL	0x02
#define	FP_SUBNORMAL	0x03
#define	FP_ZERO		0x04
/* NetBSD extensions */
#define	_FP_LOMD	0x80		/* range for machine-specific classes */
#define	_FP_HIMD	0xff

#endif /* !_ANSI_SOURCE && ... */

/*
 * XOPEN/SVID
 */
#if defined(_XOPEN_SOURCE) || defined(__BSD_VISIBLE)
#define	M_E		    2.7182818284590452354	/* e */
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
#endif /* _XOPEN_SOURCE || __BSD_VISIBLE */

#define	HUGE	    1.701411733192644270e38
#define	LOGHUGE	    39

__BEGIN_DECLS

extern	double	fabs(double), floor(double), ceil(double), fmod(double, double), ldexp(double, int);
extern	double	sqrt(double), hypot(double, double), atof(const char *);
extern	double	sin(double), cos(double), tan(double), asin(double), acos(double), atan(double), atan2(double, double);
extern	double	exp(double), log(double), log10(double), pow(double, double);
extern	double	sinh(double), cosh(double), tanh(double);
extern	double	gamma(double);
extern	double	j0(double), j1(double), jn(int, double), y0(double), y1(double), yn(int, double);

/* libc/gen/arch/"arch" */
double frexp(double, int *);
int    isnan(double);
int    isinf(double);
__END_DECLS

#endif /* _MATH_H_ */
