/*	$NetBSD: tzfile.h,v 1.7 2003/08/07 09:44:11 agc Exp $	*/

/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Arthur David Olson of the National Cancer Institute.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *
 *	@(#)tzfile.h	8.1 (Berkeley) 6/2/93
 */
/*
 *	@(#)tzfile.h	5.2.1 (2.11BSD) 1996/11/29
 */

#ifndef _TZFILE_H_
#define	_TZFILE_H_

/*
** Information about time zone files.
*/

			/* Time zone object file directory */
#define	TZDIR		"/usr/share/zoneinfo"
#define	TZDEFAULT	"/etc/localtime"
#define TZDEFRULES	"posixrules"

/*
** Each file begins with. . .
*/

#define TZ_MAGIC	"TZif"

struct tzhead {
	char	tzh_magic[4];		/* TZ_MAGIC */
	char	tzh_reserved[32];	/* reserved for future use */
	char	tzh_ttisgmtcnt[4];	/* coded number of trans. time flags */
	char	tzh_ttisstdcnt[4];	/* coded number of trans. time flags */
	char	tzh_leapcnt[4];		/* coded number of leap seconds */
	char	tzh_timecnt[4];		/* coded number of transition times */
	char	tzh_typecnt[4];		/* coded number of local time types */
	char	tzh_charcnt[4];		/* coded number of abbr. chars */
};

/*
** . . .followed by. . .
**
**	tzh_timecnt (char [4])s		coded transition times a la time(2)
**	tzh_timecnt (unsigned char)s	types of local time starting at above
**	tzh_typecnt repetitions of
**		one (char [4])		coded GMT offset in seconds
**		one (unsigned char)	used to set tm_isdt
**		one (unsigned char)	that's an abbreviation list index
**	tzh_charcnt (char)s		'\0'-terminated zone abbreviaton strings
*/

/*
** In the current implementation, "tzset()" refuses to deal with files that
** exceed any of the limits below.
*/

/*
** The TZ_MAX_TIMES value below is enough to handle a bit more than a
** year's worth of solar time (corrected daily to the nearest second) or
** 138 years of Pacific Presidential Election time
** (where there are three time zone transitions every fourth year).
*/
#define	TZ_MAX_TIMES	370

//#define	NOSOLAR				/* We currently don't handle solar time */

#ifndef	NOSOLAR
#define	TZ_MAX_TYPES	256	/* Limited by what (unsigned char)'s can hold */
#else /* !NOSOLAR */
#define	TZ_MAX_TYPES	20	/* Maximum number of local time types */
#endif /* !NOSOLAR */

#define	TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */

#define	TZ_MAX_LEAPS	50	/* Maximum number of leap second corrections */

/* 211BSD & SysV Compatibility */
#define	SECS_PER_MIN	60
#define	MINS_PER_HOUR	60
#define	HOURS_PER_DAY	24
#define	DAYS_PER_WEEK	7
#define	DAYS_PER_NYEAR	365
#define	DAYS_PER_LYEAR	366
#define	SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define	SECS_PER_DAY	((long) SECS_PER_HOUR * HOURS_PER_DAY)
#define	MONS_PER_YEAR	12

/* BSD Compatibility */
#define	SECSPERMIN		SECS_PER_MIN
#define	MINSPERHOUR		MINS_PER_HOUR
#define	HOURSPERDAY		HOURS_PER_DAY
#define	DAYSPERWEEK		DAYS_PER_WEEK
#define	DAYSPERNYEAR	DAYS_PER_NYEAR
#define	DAYSPERLYEAR	DAYS_PER_LYEAR
#define	SECSPERHOUR		SECS_PER_HOUR
#define	SECSPERDAY		SECS_PER_DAY
#define	MONSPERYEAR		MONS_PER_YEAR

#define	TM_SUNDAY		0
#define	TM_MONDAY		1
#define	TM_TUESDAY		2
#define	TM_WEDNESDAY	3
#define	TM_THURSDAY		4
#define	TM_FRIDAY		5
#define	TM_SATURDAY		6

#define	TM_JANUARY		0
#define	TM_FEBRUARY		1
#define	TM_MARCH		2
#define	TM_APRIL		3
#define	TM_MAY			4
#define	TM_JUNE			5
#define	TM_JULY			6
#define	TM_AUGUST		7
#define	TM_SEPTEMBER	8
#define	TM_OCTOBER		9
#define	TM_NOVEMBER		10
#define	TM_DECEMBER		11
#define	TM_SUNDAY		0

#define	TM_YEAR_BASE	1900

#define	EPOCH_YEAR		1970
#define	EPOCH_WDAY		TM_THURSDAY

/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define	isleap(y) 		((((y) % 4) == 0 && ((y) % 100) != 0) || (((y) % 400) == 0))

/*
** Since everything in isleap is modulo 400 (or a factor of 400), we know that
**	isleap(y) == isleap(y % 400)
** and so
**	isleap(a + b) == isleap((a + b) % 400)
** or
**	isleap(a + b) == isleap(a % 400 + b % 400)
** This is true even if % means modulo rather than Fortran remainder
** (which is allowed by C89 but not C99).
** We use this to avoid addition overflow problems.
*/

#define isleap_sum(a, b) 	isleap((a) % 400 + (b) % 400)

#endif /* !_TZFILE_H_ */
