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
#if 0
static char sccsid[] = "@(#)localeconv.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include <locale.h>

#include "lmonetary.h"
#include "lnumeric.h"
#include "setlocale.h"

static void numeric_lconv(struct lconv *);
static void	monetary_lconv(struct lconv *);

extern int __monetary_locale_changed;
extern int __numeric_locale_changed;

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

#define LCONV_ASSIGNC(lc, ptr, name)   ((lc)->name = __UNCONST((ptr)->name))
#define LCONV_ASSIGN(lc, ptr, name)   ((lc)->name = (ptr)->name)

static void
numeric_lconv(lconv)
	struct lconv *lconv;
{
	if (__numeric_locale_changed) {
		numeric_locale_t *nptr;

		nptr = __get_current_numeric_locale();

		LCONV_ASSIGNC(lconv, nptr, decimal_point);
		LCONV_ASSIGNC(lconv, nptr, thousands_sep);
		LCONV_ASSIGNC(lconv, nptr, grouping);

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

		LCONV_ASSIGNC(lconv, mptr, int_curr_symbol);
		LCONV_ASSIGNC(lconv, mptr, currency_symbol);
		LCONV_ASSIGNC(lconv, mptr, mon_decimal_point);
		LCONV_ASSIGNC(lconv, mptr, mon_thousands_sep);
		LCONV_ASSIGNC(lconv, mptr, mon_grouping);
		LCONV_ASSIGNC(lconv, mptr, positive_sign);
		LCONV_ASSIGNC(lconv, mptr, negative_sign);
		LCONV_ASSIGN(lconv, mptr, int_frac_digits);
		LCONV_ASSIGN(lconv, mptr, frac_digits);
		LCONV_ASSIGN(lconv, mptr, p_cs_precedes);
		LCONV_ASSIGN(lconv, mptr, p_sep_by_space);
		LCONV_ASSIGN(lconv, mptr, n_cs_precedes);
		LCONV_ASSIGN(lconv, mptr, n_sep_by_space);
		LCONV_ASSIGN(lconv, mptr, p_sign_posn);
		LCONV_ASSIGN(lconv, mptr, n_sign_posn);
		LCONV_ASSIGN(lconv, mptr, int_p_cs_precedes);
		LCONV_ASSIGN(lconv, mptr, int_n_cs_precedes);
		LCONV_ASSIGN(lconv, mptr, int_p_sep_by_space);
		LCONV_ASSIGN(lconv, mptr, int_n_sep_by_space);
		LCONV_ASSIGN(lconv, mptr, int_p_sign_posn);
		LCONV_ASSIGN(lconv, mptr, int_n_sign_posn);

		__monetary_locale_changed = 0;
	}
}
