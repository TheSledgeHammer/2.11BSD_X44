/*
 * Copyright (c) 2000, 2001 Alexey Zelkin <phantom@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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
__FBSDID("$FreeBSD$");
#endif

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include "ldpart.h"
#include "lmonetary.h"

extern int __monetary_locale_changed;

#define LCMONETARY_SIZE_FULL (sizeof(monetary_locale_t) / sizeof(char *))
#define LCMONETARY_SIZE_MIN  (offsetof(monetary_locale_t, int_p_cs_precedes) / sizeof(char *))

static char	empty[] = "";

static const monetary_locale_t _C_monetary_locale = {
		empty,		/* int_curr_symbol */
		empty,		/* currency_symbol */
		empty,		/* mon_decimal_point */
		empty,		/* mon_thousands_sep */
		empty,	    /* mon_grouping */
		empty,		/* positive_sign */
		empty,		/* negative_sign */
		CHAR_MAX,	/* int_frac_digits */
		CHAR_MAX,	/* frac_digits */
		CHAR_MAX,	/* p_cs_precedes */
		CHAR_MAX,	/* p_sep_by_space */
		CHAR_MAX,	/* n_cs_precedes */
		CHAR_MAX,	/* n_sep_by_space */
		CHAR_MAX,	/* p_sign_posn */
		CHAR_MAX,	/* n_sign_posn */
		CHAR_MAX,	/* int_p_cs_precedes */
		CHAR_MAX,	/* int_n_cs_precedes */
		CHAR_MAX,	/* int_p_sep_by_space */
		CHAR_MAX,	/* int_n_sep_by_space */
		CHAR_MAX,	/* int_p_sign_posn */
		CHAR_MAX	/* int_n_sign_posn */
};

static monetary_locale_t _monetary_locale;
static int	_monetary_using_locale;
static char	*_monetary_locale_buf;

int
__monetary_load_locale(const char *name)
{
	int ret;

	ret = __part_load_locale(name, &_monetary_using_locale,
		&_monetary_locale_buf, "LC_MONETARY",
		LCMONETARY_SIZE_FULL, LCMONETARY_SIZE_MIN,
		(const char **)&_monetary_locale);
	if (ret != _LDP_ERROR)
		__monetary_locale_changed = 1;
	if (ret == _LDP_LOADED) {
		_monetary_locale.mon_grouping =
		     __fix_locale_grouping_str(_monetary_locale.mon_grouping);
#ifdef notyet
#define M_ASSIGN_CHAR(NAME) (((char *)_monetary_locale.NAME)[0] = \
			     cnv(_monetary_locale.NAME))

		M_ASSIGN_CHAR(int_frac_digits);
		M_ASSIGN_CHAR(frac_digits);
		M_ASSIGN_CHAR(p_cs_precedes);
		M_ASSIGN_CHAR(p_sep_by_space);
		M_ASSIGN_CHAR(n_cs_precedes);
		M_ASSIGN_CHAR(n_sep_by_space);
		M_ASSIGN_CHAR(p_sign_posn);
		M_ASSIGN_CHAR(n_sign_posn);

		/*
		 * The six additional C99 international monetary formatting
		 * parameters default to the national parameters when
		 * reading FreeBSD 4 LC_MONETARY data files.
		 */
#define	M_ASSIGN_ICHAR(NAME)							\
		do {											\
			if (_monetary_locale.int_##NAME == NULL)	\
				_monetary_locale.int_##NAME =			\
				    _monetary_locale.NAME;				\
			else										\
				M_ASSIGN_CHAR(int_##NAME);				\
		} while (0)

		M_ASSIGN_ICHAR(p_cs_precedes);
		M_ASSIGN_ICHAR(n_cs_precedes);
		M_ASSIGN_ICHAR(p_sep_by_space);
		M_ASSIGN_ICHAR(n_sep_by_space);
		M_ASSIGN_ICHAR(p_sign_posn);
		M_ASSIGN_ICHAR(n_sign_posn);
#endif
	}
	return (ret);
}

monetary_locale_t *
__get_current_monetary_locale(void)
{
	return (__UNCONST(_monetary_using_locale
		? &_monetary_locale : &_C_monetary_locale));
}

#ifdef LOCALE_DEBUG
void
monetdebug() {
	printf("int_curr_symbol = %s\n"
			"currency_symbol = %s\n"
			"mon_decimal_point = %s\n"
			"mon_thousands_sep = %s\n"
			"mon_grouping = %s\n"
			"positive_sign = %s\n"
			"negative_sign = %s\n"
			"int_frac_digits = %d\n"
			"frac_digits = %d\n"
			"p_cs_precedes = %d\n"
			"p_sep_by_space = %d\n"
			"n_cs_precedes = %d\n"
			"n_sep_by_space = %d\n"
			"p_sign_posn = %d\n"
			"n_sign_posn = %d\n",
			"int_p_cs_precedes = %d\n"
			"int_p_sep_by_space = %d\n"
			"int_n_cs_precedes = %d\n"
			"int_n_sep_by_space = %d\n"
			"int_p_sign_posn = %d\n"
			"int_n_sign_posn = %d\n",
			_monetary_locale.int_curr_symbol,
			_monetary_locale.currency_symbol,
			_monetary_locale.mon_decimal_point,
			_monetary_locale.mon_thousands_sep,
			_monetary_locale.mon_grouping,
			_monetary_locale.positive_sign,
			_monetary_locale.negative_sign,
			_monetary_locale.int_frac_digits,
			_monetary_locale.frac_digits,
			_monetary_locale.p_cs_precedes,
			_monetary_locale.p_sep_by_space,
			_monetary_locale.n_cs_precedes,
			_monetary_locale.n_sep_by_space,
			_monetary_locale.p_sign_posn,
			_monetary_locale.n_sign_posn,
			_monetary_locale.int_p_cs_precedes,
			_monetary_locale.int_p_sep_by_space,
			_monetary_locale.int_n_cs_precedes,
			_monetary_locale.int_n_sep_by_space,
			_monetary_locale.int_p_sign_posn,
			_monetary_locale.int_n_sign_posn
											 );
}
#endif /* LOCALE_DEBUG */
