/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2001 Alexey Zelkin <phantom@FreeBSD.org>
 * Copyright (c) 1997 FreeBSD Inc.
 * All rights reserved.
 *
 * Copyright (c) 2011 The FreeBSD Foundation
 * All rights reserved.
 * Portions of this software were developed by David Chisnall
 * under sponsorship from the FreeBSD Foundation.
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

#include <stddef.h>
#include <stdio.h>

#include "ldpart.h"
#include "ltime.h"

#define	LCTIME_SIZE (sizeof(time_locale_t) / sizeof(char *))

static const time_locale_t	_C_time_locale = {
		{
				"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		},
		{
				"January", "February", "March", "April", "May", "June",
				"July", "August", "September", "October", "November", "December"
		},
		{
				"Sun", "Mon", "Tue", "Wed",
				"Thu", "Fri", "Sat"
		},
		{
				"Sunday", "Monday", "Tuesday", "Wednesday",
				"Thursday", "Friday", "Saturday"
		},
		/* X_fmt */
		"%H:%M:%S",

		/*
		 * x_fmt
		 * Since the C language standard calls for
	 	 * "date, using locale's date format," anything goes.
	 	 * Using just numbers (as here) makes Quakers happier;
	 	 * it's also compatible with SVR4.
	 	 */
		"%m/%d/%y",

		/*
		 * c_fmt
		 */
		"%a %b %e %H:%M:%S %Y",

		/* am */
		"AM",

		/* pm */
		"PM",

		/* date_fmt */
		"%a %b %e %H:%M:%S %Z %Y",

		/* alt_month
		 * Standalone months forms for %OB
		 */
		{
				"January", "February", "March", "April", "May", "June",
				"July", "August", "September", "October", "November", "December"
		},

		/* md_order
		 * Month / day order in dates
		 */
		"md",

		/* ampm_fmt
		 * To determine 12-hour clock format time (empty, if N/A)
		 */
		"%I:%M:%S %p"
};

static time_locale_t _time_locale;
static int	_time_using_locale;
static char	*_time_locale_buf;

int
__time_load_locale(const char *name)
{
	int ret;

	ret = __part_load_locale(name, &_time_using_locale, &_time_locale_buf,
			"LC_TIME", LCTIME_SIZE, LCTIME_SIZE,
			(const char**) &_time_locale);
	return (ret);
}

time_locale_t *
__get_current_time_locale(void)
{
	return (__UNCONST(_time_using_locale ? &_time_locale : &_C_time_locale));
}
