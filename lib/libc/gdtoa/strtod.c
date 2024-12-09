/*	$NetBSD: strtod.c,v 1.45.2.1 2005/04/19 13:35:54 tron Exp $	*/

/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991 by AT&T.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR AT&T MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

/* Please send bug reports to
	David M. Gay
	AT&T Bell Laboratories, Room 2C-463
	600 Mountain Avenue
	Murray Hill, NJ 07974-2070
	U.S.A.
	dmg@research.att.com or research!dmg
 */

/* strtod for IEEE-, VAX-, and IBM-arithmetic machines.
 *
 * This strtod returns a nearest machine number to the input decimal
 * string (or sets errno to ERANGE).  With IEEE arithmetic, ties are
 * broken by the IEEE round-even rule.  Otherwise ties are broken by
 * biased rounding (add half and chop).
 *
 * Inspired loosely by William D. Clinger's paper "How to Read Floating
 * Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].
 *
 * Modifications:
 *
 *	1. We only require IEEE, IBM, or VAX double-precision
 *		arithmetic (not IEEE double-extended).
 *	2. We get by with floating-point arithmetic in a case that
 *		Clinger missed -- when we're computing d * 10^n
 *		for a small integer d and the integer n is not too
 *		much larger than 22 (the maximum integer k for which
 *		we can represent 10^k exactly), we may be able to
 *		compute (d*10^k) * 10^(e-k) with just one roundoff.
 *	3. Rather than a bit-at-a-time adjustment of the binary
 *		result in the hard case, we use floating-point
 *		arithmetic to determine the adjustment to within
 *		one bit; only in really hard cases do we need to
 *		compute a second residual.
 *	4. Because of 3., we don't need a large table of powers of 10
 *		for ten-to-e (just some small tables, e.g. of 10^k
 *		for 0 <= k <= 22).
 */

/*
 * #define IEEE_LITTLE_ENDIAN for IEEE-arithmetic machines where the least
 *	significant byte has the lowest address.
 * #define IEEE_BIG_ENDIAN for IEEE-arithmetic machines where the most
 *	significant byte has the lowest address.
 * #define Long int on machines with 32-bit ints and 64-bit longs.
 * #define Sudden_Underflow for IEEE-format machines without gradual
 *	underflow (i.e., that flush to zero on underflow).
 * #define IBM for IBM mainframe-style floating-point arithmetic.
 * #define VAX for VAX-style floating-point arithmetic.
 * #define Unsigned_Shifts if >> does treats its left operand as unsigned.
 * #define No_leftright to omit left-right logic in fast floating-point
 *	computation of dtoa.
 * #define Check_FLT_ROUNDS if FLT_ROUNDS can assume the values 2 or 3.
 * #define RND_PRODQUOT to use rnd_prod and rnd_quot (assembly routines
 *	that use extended-precision instructions to compute rounded
 *	products and quotients) with IBM.
 * #define ROUND_BIASED for IEEE-format with biased rounding.
 * #define Inaccurate_Divide for IEEE-format with correctly rounded
 *	products but inaccurate quotients, e.g., for Intel i860.
 * #define Just_16 to store 16 bits per 32-bit Long when doing high-precision
 *	integer arithmetic.  Whether this speeds things up or slows things
 *	down depends on the machine and the number being converted.
 * #define KR_headers for old-style C function headers.
 * #define Bad_float_h if your system lacks a float.h or if it does not
 *	define some or all of DBL_DIG, DBL_MAX_10_EXP, DBL_MAX_EXP,
 *	FLT_RADIX, FLT_ROUNDS, and DBL_MAX.
 * #define MALLOC your_malloc, where your_malloc(n) acts like malloc(n)
 *	if memory is available and otherwise does something you deem
 *	appropriate.  If MALLOC is undefined, malloc will be invoked
 *	directly -- and assumed always to succeed.
 */

#include "namespace.h"
#include "gdtoaimp.h"

double
strtod
#ifdef KR_headers
	(s00, se) CONST char *s00; char **se;
#else
	(CONST char *s00, char **se)
#endif
{
	int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, dsign,
		 e, e1, esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
	CONST char *s, *s0, *s1;
	double aadj, aadj1, adj;
	_double rv, rv0;
	Long L;
	ULong y, z;
	Bigint *bb1, *bd0;
	Bigint *bb = NULL, *bd = NULL, *bs = NULL, *delta = NULL;/* pacify gcc */

#ifndef KR_headers
	CONST char decimal_point = localeconv()->decimal_point[0];
#else
	CONST char decimal_point = '.';
#endif

	sign = nz0 = nz = 0;
	value(rv) = 0.;

	for (s = s00; isspace((unsigned char) *s); s++)
		;

	if (*s == '-') {
		sign = 1;
		s++;
	} else if (*s == '+') {
		s++;
	}

	if (*s == '\0') {
		s = s00;
		goto ret;
	}

	/* "INF" or "INFINITY" */
	if (tolower((unsigned char) *s) == 'i' && strncasecmp(s, "inf", 3) == 0) {
		if (strncasecmp(s + 3, "inity", 5) == 0)
			s += 8;
		else
			s += 3;

		value(rv) = HUGE_VAL;
		goto ret;
	}

#ifdef IEEE_Arith
	/* "NAN" or "NAN(n-char-sequence-opt)" */
	if (tolower((unsigned char) *s) == 'n' && strncasecmp(s, "nan", 3) == 0) {
		/* Build a quiet NaN. */
		word0(rv) = NAN_WORD0;
		word1(rv) = NAN_WORD1;
		s += 3;

		/* Don't interpret (n-char-sequence-opt), for now. */
		if (*s == '(') {
			s0 = s;
			for (s++; *s != ')' && *s != '\0'; s++)
				;
			if (*s == ')')
				s++; /* Skip over closing paren ... */
			else
				s = s0; /* ... otherwise go back. */
		}

		goto ret;
	}
#endif

	if (*s == '0') {
		nz0 = 1;
		while (*++s == '0')
			;
		if (!*s)
			goto ret;
	}
	s0 = s;
	y = z = 0;
	for (nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
		if (nd < 9)
			y = 10 * y + c - '0';
		else if (nd < 16)
			z = 10 * z + c - '0';
	nd0 = nd;
	if (c == decimal_point) {
		c = *++s;
		if (!nd) {
			for (; c == '0'; c = *++s)
				nz++;
			if (c > '0' && c <= '9') {
				s0 = s;
				nf += nz;
				nz = 0;
				goto have_dig;
			}
			goto dig_done;
		}
		for (; c >= '0' && c <= '9'; c = *++s) {
 have_dig:
			nz++;
			if (c -= '0') {
				nf += nz;
				for (i = 1; i < nz; i++)
					if (nd++ < 9)
						y *= 10;
					else if (nd <= DBL_DIG + 1)
						z *= 10;
				if (nd++ < 9)
					y = 10 * y + c;
				else if (nd <= DBL_DIG + 1)
					z = 10 * z + c;
				nz = 0;
			}
		}
	}
 dig_done:
	e = 0;
	if (c == 'e' || c == 'E') {
		if (!nd && !nz && !nz0) {
			s = s00;
			goto ret;
		}
		s00 = s;
		esign = 0;
		switch (c = *++s) {
		case '-':
			esign = 1;
			/* FALLTHROUGH */
		case '+':
			c = *++s;
		}
		if (c >= '0' && c <= '9') {
			while (c == '0')
				c = *++s;
			if (c > '0' && c <= '9') {
				L = c - '0';
				s1 = s;
				while ((c = *++s) >= '0' && c <= '9')
					L = 10 * L + c - '0';
				if (s - s1 > 8 || L > 19999)
					/* Avoid confusion from exponents
					 * so large that e might overflow.
					 */
					e = 19999; /* safe for 16 bit ints */
				else
					e = (int) L;
				if (esign)
					e = -e;
			} else
				e = 0;
		} else
			s = s00;
	}
	if (!nd) {
		if (!nz && !nz0)
			s = s00;
		goto ret;
	}
	e1 = e -= nf;

	/* Now we have nd0 digits, starting at s0, followed by a
	 * decimal point, followed by nd-nd0 digits.  The number we're
	 * after is the integer represented by those digits times
	 * 10**e */

	if (!nd0)
		nd0 = nd;
	k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
	value(rv) = y;
	if (k > 9)
		value(rv) = tens[k - 9] * value(rv) + z;
	bd0 = 0;
	if (nd <= DBL_DIG
#ifndef RND_PRODQUOT
			&& FLT_ROUNDS == 1
#endif
					) {
		if (!e)
			goto ret;
		if (e > 0) {
			if (e <= Ten_pmax) {
#ifdef VAX
				goto vax_ovfl_check;
#else
				/* value(rv) = */rounded_product(value(rv), tens[e]);
				goto ret;
#endif
			}
			i = DBL_DIG - nd;
			if (e <= Ten_pmax + i) {
				/* A fancier test would sometimes let us do
				 * this for larger i values.
				 */
				e -= i;
				value(rv) *= tens[i];
#ifdef VAX
				/* VAX exponent range is so narrow we must
				 * worry about overflow here...
				 */
 vax_ovfl_check:
				word0(rv) -= P * Exp_msk1;
				/* value(rv) = */rounded_product(value(rv), tens[e]);
				if ((word0(rv) & Exp_mask)
						> Exp_msk1 * (DBL_MAX_EXP + Bias - 1 - P))
					goto ovfl;
				word0(rv) += P * Exp_msk1;
#else
				/* value(rv) = */rounded_product(value(rv), tens[e]);
#endif
				goto ret;
			}
		}
#ifndef Inaccurate_Divide
		else if (e >= -Ten_pmax) {
			/* value(rv) = */rounded_quotient(value(rv), tens[-e]);
			goto ret;
		}
#endif
	}
	e1 += nd - k;

	/* Get starting approximation = rv * 10**e1 */

	if (e1 > 0) {
		if ((i = e1 & 15) != 0)
			value(rv) *= tens[i];
		if (e1 &= ~15) {
			if (e1 > DBL_MAX_10_EXP) {
 ovfl:
				errno = ERANGE;
				value(rv) = HUGE_VAL;
				if (bd0)
					goto retfree;
				goto ret;
			}
			if ((e1 = (unsigned int) e1 >> 4) != 0) {
				for (j = 0; e1 > 1; j++, e1 = (unsigned int) e1 >> 1)
					if (e1 & 1)
						value(rv) *= bigtens[j];
				/* The last multiplication could overflow. */
				word0(rv) -= P * Exp_msk1;
				value(rv) *= bigtens[j];
				if ((z = word0(rv) & Exp_mask)
						> Exp_msk1 * (DBL_MAX_EXP + Bias - P))
					goto ovfl;
				if (z > Exp_msk1 * (DBL_MAX_EXP + Bias - 1 - P)) {
					/* set to largest number */
					/* (Can't trust DBL_MAX) */
					word0(rv) = Big0;
					word1(rv) = Big1;
				} else
					word0(rv) += P * Exp_msk1;
			}

		}
	} else if (e1 < 0) {
		e1 = -e1;
		if ((i = e1 & 15) != 0)
			value(rv) /= tens[i];
		if (e1 &= ~15) {
			e1 = (unsigned int) e1 >> 4;
			if (e1 >= 1 << n_bigtens)
				goto undfl;
			for (j = 0; e1 > 1; j++, e1 = (unsigned int) e1 >> 1)
				if (e1 & 1)
					value(rv) *= tinytens[j];
			/* The last multiplication could underflow. */
			value(rv0) = value(rv);
			value(rv) *= tinytens[j];
			if (!value(rv)) {
				value(rv) = 2. * value(rv0);
				value(rv) *= tinytens[j];
				if (!value(rv)) {
 undfl:
					value(rv) = 0.;
					errno = ERANGE;
					if (bd0)
						goto retfree;
					goto ret;
				}
				word0(rv) = Tiny0;
				word1(rv) = Tiny1;
				/* The refinement below will clean
				 * this approximation up.
				 */
			}
		}
	}

	/* Now the hard part -- adjusting rv to the correct value.*/

	/* Put digits into bd: true value = bd * 10^e */

	bd0 = s2b(s0, nd0, nd, y);

	for (;;) {
		bd = Balloc(bd0->k);
		Bcopy(bd, bd0);
		bb = d2b(value(rv), &bbe, &bbbits); /* rv = bb * 2^bbe */
		bs = i2b(1);

		if (e >= 0) {
			bb2 = bb5 = 0;
			bd2 = bd5 = e;
		} else {
			bb2 = bb5 = -e;
			bd2 = bd5 = 0;
		}
		if (bbe >= 0)
			bb2 += bbe;
		else
			bd2 -= bbe;
		bs2 = bb2;
#ifdef Sudden_Underflow
#ifdef IBM
		j = 1 + 4*P - 3 - bbbits + ((bbe + bbbits - 1) & 3);
#else
		j = P + 1 - bbbits;
#endif
#else
		i = bbe + bbbits - 1; /* logb(rv) */
		if (i < Emin) /* denormal */
			j = bbe + (P - Emin);
		else
			j = P + 1 - bbbits;
#endif
		bb2 += j;
		bd2 += j;
		i = bb2 < bd2 ? bb2 : bd2;
		if (i > bs2)
			i = bs2;
		if (i > 0) {
			bb2 -= i;
			bd2 -= i;
			bs2 -= i;
		}
		if (bb5 > 0) {
			bs = pow5mult(bs, bb5);
			bb1 = mult(bs, bb);
			Bfree(bb);
			bb = bb1;
		}
		if (bb2 > 0)
			bb = lshift(bb, bb2);
		if (bd5 > 0)
			bd = pow5mult(bd, bd5);
		if (bd2 > 0)
			bd = lshift(bd, bd2);
		if (bs2 > 0)
			bs = lshift(bs, bs2);
		delta = diff(bb, bd);
		dsign = delta->sign;
		delta->sign = 0;
		i = cmp(delta, bs);
		if (i < 0) {
			/* Error is less than half an ulp -- check for
			 * special case of mantissa a power of two.
			 */
			if (dsign || word1(rv) || word0(rv) & Bndry_mask)
				break;
			delta = lshift(delta, Log2P);
			if (cmp(delta, bs) > 0)
				goto drop_down;
			break;
		}
		if (i == 0) {
			/* exactly half-way between */
			if (dsign) {
				if ((word0(rv) & Bndry_mask1) == Bndry_mask1
						&& word1(rv) == 0xffffffff) {
					/*boundary case -- increment exponent*/
					word0(rv) = (word0(rv) & Exp_mask) + Exp_msk1
#ifdef IBM
							| Exp_msk1 >> 4
#endif
									;
					word1(rv) = 0;
					break;
				}
			} else if (!(word0(rv) & Bndry_mask) && !word1(rv)) {
 drop_down:
				/* boundary case -- decrement exponent */
#ifdef Sudden_Underflow
				L = word0(rv) & Exp_mask;
#ifdef IBM
				if (L <  Exp_msk1)
#else
				if (L <= Exp_msk1)
#endif
					goto undfl;
				L -= Exp_msk1;
#else
				L = (word0(rv) & Exp_mask) - Exp_msk1;
#endif
				word0(rv) = L | Bndry_mask1;
				word1(rv) = 0xffffffff;
#ifdef IBM
				goto cont;
#else
				break;
#endif
			}
#ifndef ROUND_BIASED
			if (!(word1(rv) & LSB))
				break;
#endif
			if (dsign)
				value(rv) += ulp(value(rv));
#ifndef ROUND_BIASED
			else {
				value(rv) -= ulp(value(rv));
#ifndef Sudden_Underflow
				if (!value(rv))
					goto undfl;
#endif
			}
#endif
			break;
		}
		if ((aadj = ratio(delta, bs)) <= 2.) {
			if (dsign)
				aadj = aadj1 = 1.;
			else if (word1(rv) || word0(rv) & Bndry_mask) {
#ifndef Sudden_Underflow
				if (word1(rv) == Tiny1 && !word0(rv))
					goto undfl;
#endif
				aadj = 1.;
				aadj1 = -1.;
			} else {
				/* special case -- power of FLT_RADIX to be */
				/* rounded down... */

				if (aadj < 2. / FLT_RADIX)
					aadj = 1. / FLT_RADIX;
				else
					aadj *= 0.5;
				aadj1 = -aadj;
			}
		} else {
			aadj *= 0.5;
			aadj1 = dsign ? aadj : -aadj;
#ifdef Check_FLT_ROUNDS
			switch (FLT_ROUNDS) {
			case 2: /* towards +infinity */
				aadj1 -= 0.5;
				break;
			case 0: /* towards 0 */
			case 3: /* towards -infinity */
				aadj1 += 0.5;
			}
#else
			if (FLT_ROUNDS == 0)
				aadj1 += 0.5;
#endif
		}
		y = word0(rv) & Exp_mask;

		/* Check for overflow */

		if (y == Exp_msk1 * (DBL_MAX_EXP + Bias - 1)) {
			value(rv0) = value(rv);
			word0(rv) -= P * Exp_msk1;
			adj = aadj1 * ulp(value(rv));
			value(rv) += adj;
			if ((word0(rv) & Exp_mask) >=
			Exp_msk1 * (DBL_MAX_EXP + Bias - P)) {
				if (word0(rv0) == Big0 && word1(rv0) == Big1)
					goto ovfl;
				word0(rv) = Big0;
				word1(rv) = Big1;
				goto cont;
			} else
				word0(rv) += P * Exp_msk1;
		} else {
#ifdef Sudden_Underflow
			if ((word0(rv) & Exp_mask) <= P * Exp_msk1) {
				value(rv0) = value(rv);
				word0(rv) += P * Exp_msk1;
				adj = aadj1 * ulp(value(rv));
				value(rv) += adj;
#ifdef IBM
				if ((word0(rv) & Exp_mask) <  P*Exp_msk1)
#else
				if ((word0(rv) & Exp_mask) <= P * Exp_msk1)
#endif
				{
					if (word0(rv0) == Tiny0 && word1(rv0) == Tiny1)
						goto undfl;
					word0(rv) = Tiny0;
					word1(rv) = Tiny1;
					goto cont;
				} else
					word0(rv) -= P * Exp_msk1;
			} else {
				adj = aadj1 * ulp(value(rv));
				value(rv) += adj;
			}
#else
			/* Compute adj so that the IEEE rounding rules will
			 * correctly round rv + adj in some half-way cases.
			 * If rv * ulp(rv) is denormalized (i.e.,
			 * y <= (P-1)*Exp_msk1), we must adjust aadj to avoid
			 * trouble from bits lost to denormalization;
			 * example: 1.2e-307 .
			 */
			if (y <= (P - 1) * Exp_msk1 && aadj >= 1.) {
				aadj1 = (double) (int) (aadj + 0.5);
				if (!dsign)
					aadj1 = -aadj1;
			}
			adj = aadj1 * ulp(value(rv));
			value(rv) += adj;
#endif
		}
		z = word0(rv) & Exp_mask;
		if (y == z) {
			/* Can we stop now? */
			L = aadj;
			aadj -= L;
			/* The tolerances below are conservative. */
			if (dsign || word1(rv) || word0(rv) & Bndry_mask) {
				if (aadj < .4999999 || aadj > .5000001)
					break;
			} else if (aadj < .4999999 / FLT_RADIX)
				break;
		}
 cont:
		Bfree(bb);
		Bfree(bd);
		Bfree(bs);
		Bfree(delta);
	}
 retfree:
	Bfree(bb);
	Bfree(bd);
	Bfree(bs);
	Bfree(bd0);
	Bfree(delta);
 ret:
	if (se)
		/* LINTED interface specification */
        *se = __UNCONST(s);
	return sign ? -value(rv) : value(rv);
}
