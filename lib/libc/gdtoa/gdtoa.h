/* $NetBSD: gdtoa.h,v 1.6.4.1.4.1 2008/04/08 21:10:55 jdc Exp $ */

/****************************************************************

The author of this software is David M. Gay.

Copyright (C) 1998 by Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

****************************************************************/

/* Please send bug reports to David M. Gay (dmg at acm dot org,
 * with " at " changed at "@" and " dot " changed to ".").	*/

#ifndef GDTOA_H_INCLUDED
#define GDTOA_H_INCLUDED

#include "arith.h"
#include <stddef.h> /* for size_t */
#include <stdint.h>

#ifndef __LOCALE_T_DECLARED
typedef struct _locale		*locale_t;
#define __LOCALE_T_DECLARED
#endif

#ifndef Long
#define Long int32_t
#endif
#ifndef ULong
#define ULong uint32_t
#endif
#ifndef UShort
#define UShort uint16_t
#endif

#ifndef ANSI
#ifdef KR_headers
#define ANSI(x) ()
#define Void /*nothing*/
#else
#define ANSI(x) x
#define Void void
#endif
#endif /* ANSI */

#ifndef CONST
#ifdef KR_headers
#define CONST /* blank */
#else
#define CONST const
#endif
#endif /* CONST */

 enum {	/* return values from strtodg */
	STRTOG_Zero	= 0,
	STRTOG_Normal	= 1,
	STRTOG_Denormal	= 2,
	STRTOG_Infinite	= 3,
	STRTOG_NaN	= 4,
	STRTOG_NaNbits	= 5,
	STRTOG_NoNumber	= 6,
	STRTOG_Retmask	= 7,

	/* The following may be or-ed into one of the above values. */

	STRTOG_Neg		= 0x08, /* does not affect STRTOG_Inexlo or STRTOG_Inexhi */
	STRTOG_Inexlo	= 0x10,	/* returned result rounded toward zero */
	STRTOG_Inexhi	= 0x20, /* returned result rounded away from zero */
	STRTOG_Inexact	= 0x30,
	STRTOG_Underflow= 0x40,
	STRTOG_Overflow	= 0x80,
	STRTOG_NoMemory = 0x100
};

typedef struct
FPI {
	int nbits;
	int emin;
	int emax;
	int rounding;
	int sudden_underflow;
} FPI;

enum {	/* FPI.rounding values: same as FLT_ROUNDS */
	FPI_Round_zero = 0,
	FPI_Round_near = 1,
	FPI_Round_up = 2,
	FPI_Round_down = 3
};

#ifdef __cplusplus
extern "C" {
#endif

#define	dtoa		__dtoa

extern char *dtoa ANSI((double, int, int, int *, int *, char **));
extern float  strtof ANSI((CONST char *, char **));
extern double strtod ANSI((CONST char *, char **));
extern int strtodg ANSI((CONST char *, char **, CONST FPI *, Long *, ULong *));

extern int	strtord  ANSI((CONST char *, char **, int, double *));
extern int 	strtordd ANSI((CONST char *, char **, int, double *));
extern int	strtorf  ANSI((CONST char *, char **, int, float *));
extern int	strtorx  ANSI((CONST char *, char **, int, void *));
#if 1
extern int	strtopd  ANSI((CONST char *, char **, double *));
extern int 	strtopdd ANSI((CONST char *, char **, double *));
extern int	strtopf  ANSI((CONST char *, char **, float *));
extern int	strtopx  ANSI((CONST char *, char **, void *));
#else
#define strtopd(s,se,x) strtord(s,se,1,x)
#define strtopdd(s,se,x)strtordd(s,se,1,x)
#define strtopf(s,se,x) strtorf(s,se,1,x)
#define strtopx(s,se,x) strtorx(s,se,1,x)
#endif

#ifdef __cplusplus
}
#endif
#endif /* GDTOA_H_INCLUDED */
