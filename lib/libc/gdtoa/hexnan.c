/* $NetBSD: hexnan.c,v 1.3 2006/03/11 18:38:14 kleink Exp $ */

/****************************************************************

The author of this software is David M. Gay.

Copyright (C) 2000 by Lucent Technologies
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

#include "gdtoaimp.h"

static void
#ifdef KR_headers
L_shift(x, x1, i)
	ULong *x; ULong *x1; int i;
#else
L_shift(ULong *x, ULong *x1, int i)
#endif
{
	int j;

	i = 8 - i;
	i <<= 2;
	j = ULbits - i;
	do {
		*x |= x[1] << j;
		x[1] >>= i;
	} while (++x < x1);
}

int
#ifdef KR_headers
hexnan(sp, fpi, x0)
	CONST char **sp; CONST FPI *fpi; ULong *x0;
#else
hexnan( CONST char **sp, CONST FPI *fpi, ULong *x0)
#endif
{
	ULong c, h, *x, *x1, *xe;
	CONST char *s;
	int havedig, hd0, i, nbits;

	if (!hexdig['0'])
		hexdig_init_D2A();
	nbits = fpi->nbits;
	x = x0 + ((unsigned int) nbits >> kshift);
	if (nbits & kmask)
		x++;
	*--x = 0;
	x1 = xe = x;
	havedig = hd0 = i = 0;
	s = *sp;
	while ((c = *(CONST unsigned char*) ++s) != 0) {
		if (!(h = hexdig[c])) {
			if (c <= ' ') {
				if (hd0 < havedig) {
					if (x < x1 && i < 8)
						L_shift(x, x1, i);
					if (x <= x0) {
						i = 8;
						continue;
					}
					hd0 = havedig;
					*--x = 0;
					x1 = x;
					i = 0;
				}
				continue;
			}
			if (/*(*/c == ')' && havedig) {
				*sp = s + 1;
				break;
			}
			return STRTOG_NaN;
		}
		havedig++;
		if (++i > 8) {
			if (x <= x0)
				continue;
			i = 1;
			*--x = 0;
		}
		*x = (*x << 4) | (h & 0xf);
	}
	if (!havedig)
		return STRTOG_NaN;
	if (x < x1 && i < 8)
		L_shift(x, x1, i);
	if (x > x0) {
		x1 = x0;
		do
			*x1++ = *x++;
		while (x <= xe);
		do
			*x1++ = 0;
		while (x1 <= xe);
	} else {
		/* truncate high-order word if necessary */
		if ((i = nbits & (ULbits - 1)) != 0)
			*xe &= ((ULong) 0xffffffff) >> (ULbits - i);
	}
	for (x1 = xe;; --x1) {
		if (*x1 != 0)
			break;
		if (x1 == x0) {
			*x1 = 1;
			break;
		}
	}
	return STRTOG_NaNbits;
}
