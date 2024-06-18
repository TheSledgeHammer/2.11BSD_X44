/*
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
static char sccsid[] = "@(#)localeconv.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <locale.h>

#include "lmonetary.h"
#include "lnumeric.h"
#include "setlocale.h"

static void numeric_lconv(struct lconv *);
static void	monetary_lconv(struct lconv *);

/*
 * Return the current locale conversion.
 */

struct lconv *
localeconv(void)
{
	return (localeconv_l(__get_locale()));
}

struct lconv *
localeconv_l(locale)
	locale_t locale;
{
	numeric_lconv(locale->part_lconv);
	monetary_lconv(locale->part_lconv);
	return (locale->part_lconv);
}

static void
numeric_lconv(lconv)
	struct lconv *lconv;
{
	if (__numeric_locale_changed) {
		numeric_locale_t *nptr;

		nptr = __get_current_numeric_locale();

		lconv->decimal_point = nptr->decimal_point;
		lconv->thousands_sep = nptr->thousands_sep;
		lconv->grouping = nptr->grouping;

		__numeric_locale_changed = 0;
	}
}

static void
monetary_lconv(lconv)
	struct lconv *lconv;
{
	if (__monetary_locale_changed) {
		monetary_locale_t *mptr;

		mptr = __get_current_monetary_locale();

		lconv->int_curr_symbol = mptr->int_curr_symbol;
		lconv->currency_symbol = mptr->currency_symbol;
		lconv->mon_decimal_point = mptr->mon_decimal_point;
		lconv->mon_thousands_sep = mptr->mon_thousands_sep;
		lconv->mon_grouping = mptr->mon_grouping;
		lconv->positive_sign = mptr->positive_sign;
		lconv->negative_sign = mptr->negative_sign;
		lconv->int_frac_digits = mptr->int_frac_digits;
		lconv->frac_digits = mptr->frac_digits;
		lconv->p_cs_precedes = mptr->p_cs_precedes;
		lconv->p_sep_by_space = mptr->p_sep_by_space;
		lconv->n_cs_precedes = mptr->n_cs_precedes;
		lconv->n_sep_by_space = mptr->n_sep_by_space;
		lconv->p_sign_posn = mptr->p_sign_posn;
		lconv->n_sign_posn = mptr->n_sign_posn;
		lconv->int_p_cs_precedes = mptr->int_p_cs_precedes;
		lconv->int_n_cs_precedes = mptr->int_n_cs_precedes;
		lconv->int_p_sep_by_space = mptr->int_p_sep_by_space;
		lconv->int_n_sep_by_space = mptr->int_n_sep_by_space;
		lconv->int_p_sign_posn = mptr->int_p_sign_posn;
		lconv->int_n_sign_posn = mptr->int_n_sign_posn;

		__monetary_locale_changed = 0;
	}
}
