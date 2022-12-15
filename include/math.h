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

__BEGIN_DECLS

extern	double	fabs(double), floor(double), ceil(double), fmod(double, double), ldexp(double, int);
extern	double	sqrt(double), hypot(double, double), atof(const char *);
extern	double	sin(), cos(double), tan(double), asin(double), acos(double), atan(double), atan2(double, double);
extern	double	exp(double), log(double), log10(double), pow(double, double);
extern	double	sinh(double), cosh(double), tanh(double);
extern	double	gamma(double);
extern	double	j0(double), j1(double), jn(int, double), y0(double), y1(double), yn(int, double);

#define	HUGE	1.701411733192644270e38
#define	LOGHUGE	39

__END_DECLS

#endif /* _MATH_H_ */
