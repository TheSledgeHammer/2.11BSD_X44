/*	$NetBSD: strptime.c,v 1.22 2000/12/20 20:56:34 christos Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Klaus Klein.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: strptime.c,v 1.22 2000/12/20 20:56:34 christos Exp $");
#endif

#include "namespace.h"

#include <sys/types.h>

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>

#include "locale/ltime.h"
#include "locale/setlocale.h"
#include "private.h"

#ifdef __weak_alias
__weak_alias(strptime,_strptime)
//__weak_alias(strptime_l, _strptime_l)
#endif

/*
 * We do not implement alternate representations. However, we always
 * check whether a given modifier is allowed for a certain conversion.
 */
#define ALT_E			0x01
#define ALT_O			0x02
#define	LEGAL_ALT(x)		{ if (alt_format & ~(x)) return (0); }

static	int conv_num(const unsigned char **, int *, int, int);
static time_locale_t *CurrentTimeLocale(locale_t);

extern time_t tzone;
extern int daylight;

static time_locale_t *
CurrentTimeLocale(locale)
	locale_t locale;
{
	time_locale_t *time, *ltime;

	time = __get_current_time_locale();
	ltime = (time_locale_t *)locale->part_impl[LC_TIME];
	if (ltime == time) {
		return (ltime);
	}
	return (time);
}

char *
strptime(buf, fmt, tm)
	const char *buf, *fmt;
	struct tm *tm;
{
	return (strptime_l(buf, fmt, tm, __get_locale()));
}

char *
strptime_l(buf, fmt, tm, locale)
	const char *buf, *fmt;
	struct tm *tm;
	locale_t locale;
{
	unsigned char c;
	const unsigned char *bp;
	size_t len = 0;
	int alt_format, i, split_year = 0;
    time_locale_t *time;

	bp = (const u_char *)buf;
    time = CurrentTimeLocale(locale);

	while ((c = *fmt) != '\0') {
		/* Clear `alternate' modifier prior to new conversion. */
		alt_format = 0;

		/* Eat up white-space. */
		if (isspace(c)) {
			while (isspace(*bp))
				bp++;

			fmt++;
			continue;
		}
				
		if ((c = *fmt++) != '%')
			goto literal;


again:		switch (c = *fmt++) {
		case '%':	/* "%%" is converted to "%". */
literal:
			if (c != *bp++)
				return (0);
			break;

		/*
		 * "Alternative" modifiers. Just set the appropriate flag
		 * and start over again.
		 */
		case 'E':	/* "%E?" alternative conversion modifier. */
			LEGAL_ALT(0);
			alt_format |= ALT_E;
			goto again;

		case 'O':	/* "%O?" alternative conversion modifier. */
			LEGAL_ALT(0);
			alt_format |= ALT_O;
			goto again;
			
		/*
		 * "Complex" conversion rules, implemented through recursion.
		 */
		case 'c':	/* Date and time, using the locale's format. */
			LEGAL_ALT(ALT_E);
			if (!(bp = (const u_char *)strptime((const char *)bp,
			    time->c_fmt, tm)))
				return (0);
			break;

		case 'D':	/* The date as "%m/%d/%y". */
			LEGAL_ALT(0);
			if (!(bp = (const u_char *) strptime((const char *)bp,
			    "%m/%d/%y", tm)))
				return (0);
			break;

		case 'R':	/* The time as "%H:%M". */
			LEGAL_ALT(0);
			if (!(bp = (const u_char *)strptime((const char *)bp,
			    "%H:%M", tm)))
				return (0);
			break;

		case 'r':	/* The time in 12-hour clock representation. */
			LEGAL_ALT(0);
			if (!(bp = (const u_char *)strptime((const char *)bp,
			    time->ampm_fmt, tm)))
				return (0);
			break;

		case 'T':	/* The time as "%H:%M:%S". */
			LEGAL_ALT(0);
			if (!(bp = (const u_char *)strptime((const char *)bp,
			    "%H:%M:%S", tm)))
				return (0);
			break;

		case 'X':	/* The time, using the locale's format. */
			LEGAL_ALT(ALT_E);
			if (!(bp = (const u_char *)strptime((const char *)bp,
			    time->X_fmt, tm)))
				return (0);
			break;

		case 'x':	/* The date, using the locale's format. */
			LEGAL_ALT(ALT_E);
			if (!(bp = (const u_char *)strptime((const char *)bp,
			    time->x_fmt, tm)))
				return (0);
			break;

		/*
		 * "Elementary" conversion rules.
		 */
		case 'A':	/* The day of week, using the locale's form. */
		case 'a':
			LEGAL_ALT(0);
			for (i = 0; i < 7; i++) {
				/* Full name. */
				len = strlen(time->weekday[i]);
				if (strncasecmp(time->weekday[i],
				    (const char *)bp, len) == 0)
					break;

				/* Abbreviated name. */
				len = strlen(time->wday[i]);
				if (strncasecmp(time->wday[i],
				    (const char *)bp, len) == 0)
					break;
			}

			/* Nothing matched. */
			if (i == 7)
				return (0);

			tm->tm_wday = i;
			bp += len;
			break;

		case 'B':	/* The month, using the locale's form. */
		case 'b':
		case 'h':
			LEGAL_ALT(0);
			for (i = 0; i < 12; i++) {
				/* Full name. */
				len = strlen(time->alt_month[i]);
				if (strncasecmp(time->alt_month[i],
				    (const char *)bp, len) == 0)
					break;

				len = strlen(time->month[i]);
				if (strncasecmp(time->month[i],
					(const char *)bp, len) == 0)
					break;

				/* Abbreviated name. */
				len = strlen(time->mon[i]);
				if (strncasecmp(time->mon[i],
				    (const char *)bp, len) == 0)
					break;
			}

			/* Nothing matched. */
			if (i == 12)
				return (0);

			tm->tm_mon = i;
			bp += len;
			break;

		case 'C':	/* The century number. */
			LEGAL_ALT(ALT_E);
			if (!(conv_num(&bp, &i, 0, 99)))
				return (0);

			if (split_year) {
				tm->tm_year = (tm->tm_year % 100) + (i * 100);
			} else {
				tm->tm_year = i * 100;
				split_year = 1;
			}
			break;

		case 'd':	/* The day of month. */
		case 'e':
			LEGAL_ALT(ALT_O);
			if (!(conv_num(&bp, &tm->tm_mday, 1, 31)))
				return (0);
			break;

		case 'k':	/* The hour (24-hour clock representation). */
			LEGAL_ALT(0);
			/* FALLTHROUGH */
		case 'H':
			LEGAL_ALT(ALT_O);
			if (!(conv_num(&bp, &tm->tm_hour, 0, 23)))
				return (0);
			break;

		case 'l':	/* The hour (12-hour clock representation). */
			LEGAL_ALT(0);
			/* FALLTHROUGH */
		case 'I':
			LEGAL_ALT(ALT_O);
			if (!(conv_num(&bp, &tm->tm_hour, 1, 12)))
				return (0);
			if (tm->tm_hour == 12)
				tm->tm_hour = 0;
			break;

		case 'j':	/* The day of year. */
			LEGAL_ALT(0);
			if (!(conv_num(&bp, &i, 1, 366)))
				return (0);
			tm->tm_yday = i - 1;
			break;

		case 'M':	/* The minute. */
			LEGAL_ALT(ALT_O);
			if (!(conv_num(&bp, &tm->tm_min, 0, 59)))
				return (0);
			break;

		case 'm':	/* The month. */
			LEGAL_ALT(ALT_O);
			if (!(conv_num(&bp, &i, 1, 12)))
				return (0);
			tm->tm_mon = i - 1;
			break;

		case 'p':	/* The locale's equivalent of AM/PM. */
			LEGAL_ALT(0);
			/* AM? */
			if (strcasecmp(&time->am[0],
			    (const char *)bp) == 0) {
				if (tm->tm_hour > 11)
					return (0);

				bp += strlen(&time->am[0]);
				break;
			}
			/* PM? */
			else if (strcasecmp(&time->pm[1],
			    (const char *)bp) == 0) {
				if (tm->tm_hour > 11)
					return (0);

				tm->tm_hour += 12;
				bp += strlen(&time->pm[1]);
				break;
			}

			/* Nothing matched. */
			return (0);

		case 'S':	/* The seconds. */
			LEGAL_ALT(ALT_O);
			if (!(conv_num(&bp, &tm->tm_sec, 0, 61)))
				return (0);
			break;

		case 'U':	/* The week of year, beginning on sunday. */
		case 'W':	/* The week of year, beginning on monday. */
			LEGAL_ALT(ALT_O);
			/*
			 * XXX This is bogus, as we can not assume any valid
			 * information present in the tm structure at this
			 * point to calculate a real value, so just check the
			 * range for now.
			 */
			 if (!(conv_num(&bp, &i, 0, 53)))
				return (0);
			 break;

		case 'w':	/* The day of week, beginning on sunday. */
			LEGAL_ALT(ALT_O);
			if (!(conv_num(&bp, &tm->tm_wday, 0, 6)))
				return (0);
			break;

		case 'Y':	/* The year. */
			LEGAL_ALT(ALT_E);
			if (!(conv_num(&bp, &i, 0, 9999)))
				return (0);

			tm->tm_year = i - TM_YEAR_BASE;
			break;

		case 'y':	/* The year within 100 years of the epoch. */
			LEGAL_ALT(ALT_E | ALT_O);
			if (!(conv_num(&bp, &i, 0, 99)))
				return (0);

			if (split_year) {
				tm->tm_year = ((tm->tm_year / 100) * 100) + i;
				break;
			}
			split_year = 1;
			if (i <= 68)
				tm->tm_year = i + 2000 - TM_YEAR_BASE;
			else
				tm->tm_year = i + 1900 - TM_YEAR_BASE;
			break;

		/*
		 * Miscellaneous conversions.
		 */
		case 'n':	/* Any kind of white-space. */
		case 't':
			LEGAL_ALT(0);
			while (isspace(*bp))
				bp++;
			break;


		default:	/* Unknown/unsupported conversion. */
			return (0);
		}


	}

	/* LINTED functional specification */
	return (__UNCONST(bp));
}


static int
conv_num(buf, dest, llim, ulim)
	const unsigned char **buf;
	int *dest;
	int llim, ulim;
{
	int result = 0;

	/* The limit also determines the number of valid digits. */
	int rulim = ulim;

	if (**buf < '0' || **buf > '9')
		return (0);

	do {
		result *= 10;
		result += *(*buf)++ - '0';
		rulim /= 10;
	} while ((result * 10 <= ulim) && rulim && **buf >= '0' && **buf <= '9');

	if (result < llim || result > ulim)
		return (0);

	*dest = result;
	return (1);
}
