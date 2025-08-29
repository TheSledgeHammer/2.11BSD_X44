/*
 * Copyright (c) 1987, 1989, 1993
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
/*
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)ctime.c	1.1 (Berkeley) 3/25/87";
static char sccsid[] = "@(#)ctime.c	8.2 (Berkeley) 3/20/94";
#endif
#endif /* LIBC_SCCS and not lint */

/*
** Leap second handling from Bradley White (bww@k.gp.cs.cmu.edu).
** POSIX-style TZ environment variable handling from Guy Harris
** (guy@auspex.com).
*/

#include "namespace.h"

#include <sys/param.h>

#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#include "private.h"

#ifdef __weak_alias
__weak_alias(asctime_r,_asctime_r)
__weak_alias(ctime_r,_ctime_r)
__weak_alias(gmtime_r,_gmtime_r)
__weak_alias(localtime_r,_localtime_r)
__weak_alias(offtime, _offtime)
__weak_alias(timegm, _timegm)
__weak_alias(tzset, _tzset)
#endif

#ifndef TZ_ABBR_MAX_LEN
#define TZ_ABBR_MAX_LEN	16
#endif /* !defined TZ_ABBR_MAX_LEN */

#ifndef TZ_ABBR_CHAR_SET
#define TZ_ABBR_CHAR_SET \
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :+-._"
#endif /* !defined TZ_ABBR_CHAR_SET */

#ifndef TZ_ABBR_ERR_CHAR
#define TZ_ABBR_ERR_CHAR	'_'
#endif /* !defined TZ_ABBR_ERR_CHAR */

#ifndef WILDABBR
/*
** Someone might make incorrect use of a time zone abbreviation:
**	1.	They might reference tzname[0] before calling tzset (explicitly
**		or implicitly).
**	2.	They might reference tzname[1] before calling tzset (explicitly
**		or implicitly).
**	3.	They might reference tzname[1] after setting to a time zone
**		in which Daylight Saving Time is never observed.
**	4.	They might reference tzname[0] after setting to a time zone
**		in which Standard Time is never observed.
**	5.	They might reference tm.tm_zone after calling offtime.
** What's best to do in the above cases is open to debate;
** for now, we just set things up so that in any of the five cases
** WILDABBR is used. Another possibility: initialize tzname[0] to the
** string "tzname[0] used before set", and similarly for the other cases.
** And another: initialize tzname[0] to "ERA", with an explanation in the
** manual page of what this "time zone abbreviation" means (doing this so
** that tzname[0] has the "normal" length of three characters).
*/
#define WILDABBR	"   "
#endif /* !defined WILDABBR */

static char		wildabbr[] = WILDABBR;


/*
** Some systems only handle "%.2d"; others only handle "%02d";
** "%02.2d" makes (most) everybody happy.
** At least some versions of gcc warn about the %02.2d;
** we conditionalize below to avoid the warning.
*/
/*
** All years associated with 32-bit time_t values are exactly four digits long;
** some years associated with 64-bit time_t values are not.
** Vintage programs are coded for years that are always four digits long
** and may assume that the newline always lands in the same place.
** For years that are less than four digits, we pad the output with
** leading zeroes to get the newline in the traditional place.
** The -4 ensures that we get four characters of output even if
** we call a strftime variant that produces fewer characters for some years.
** The ISO C 1999 and POSIX 1003.1-2004 standards prohibit padding the year,
** but many implementations pad anyway; most likely the standards are buggy.
*/
#define ASCTIME_FMT	"%.3s %.3s%3d %2.2d:%2.2d:%2.2d %-4d\n"
                      
/*
** For years that are more than four digits we put extra spaces before the year
** so that code trying to overwrite the newline won't end up overwriting
** a digit within a year and truncating the year (operating on the assumption
** that no output is better than wrong output).
*/
#define ASCTIME_FMT_B	"%.3s %.3s%3d %2.2d:%2.2d:%2.2d     %d\n"

#define STD_ASCTIME_BUF_SIZE	26
/*
** Big enough for something such as
** ??? ???-2147483648 -2147483648:-2147483648:-2147483648     -2147483648\n
** (two three-character abbreviations, five strings denoting integers,
** seven explicit spaces, two explicit colons, a newline,
** and a trailing ASCII nul).
** The values above are for systems where an int is 32 bits and are provided
** as an example; the define below calculates the maximum for the system at
** hand.
*/
#define MAX_ASCTIME_BUF_SIZE	(2*3+5*INT_STRLEN_MAXIMUM(int)+7+2+1+1)

static char	buf_asctime[MAX_ASCTIME_BUF_SIZE];
static const char GMT[] = "GMT";

struct ttinfo {					/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
	int		tt_ttisstd;	/* TRUE if transition is std time */
};

struct lsinfo {					/* leap second information */
	time_t		ls_trans;	/* transition time */
	int_fast32_t	ls_corr;	/* correction to apply */
};

struct state {
	int		leapcnt;
	int		timecnt;
	int		typecnt;
	int		charcnt;
	time_t		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[TZ_MAX_CHARS + 1];

	struct lsinfo	lsis[TZ_MAX_LEAPS];
};

struct rule {
	int		r_type;		/* type of rule--see below */
	int		r_day;		/* day number of rule */
	int		r_week;		/* week number of rule */
	int		r_mon;		/* month number of rule */
	long		r_time;		/* transition time of rule */
};

#define	JULIAN_DAY		0	/* Jn - Julian day */
#define	DAY_OF_YEAR		1	/* n - day of year */
#define	MONTH_NTH_DAY_OF_WEEK	2	/* Mm.n.d - month, week, day of week */

const char *tzname[2] = {
		GMT,
		GMT
		/*
		"GMT",
		"GMT"
		*/
};

static struct state	s;
static struct state *lclptr = &s;
static struct state *gmtptr = &s;

static int	lcl_is_set; /* tz_is_set */
static int	gmt_is_set;

#ifdef USG_COMPAT
time_t		tzone = 0;
int			daylight = 0;
#endif /* USG_COMPAT */

#ifdef ALTZONE
time_t		altzone = 0;
#endif /* ALTZONE */

static long 	detzcode(const char *);
static int 		tzload1(const char *);
static int 		tzload(const char *, struct state *);
static const char *getzname(const char *);
static const char *getnum(const char *, int *, int, int);
static const char *getsecs(const char *, long *);
static const char *getoffset(const char *, long *);
static const char *getrule(const char *, struct rule *);
static int 		tzsetkernel(void);
static void 	tzsetgmt(void);
static int 		tzparse(const char *, struct state *, const int);
static void 	gmtload(struct state *);
static void 	gmtcheck(void);
static struct tm *localtimesub(const time_t *, struct tm *);
static struct tm *gmttimesub(const time_t *, long, struct tm *);
static void 	localsub(const time_t *, long, struct tm *);
static void 	gmtsub(const time_t *, long, struct tm *);
static const struct state *state_alloc(void (*)(const time_t *, long, struct tm *));
static void		normalize(int *, int *, int);
static time_t	time1(struct tm *, void (*)(const time_t *, long, struct tm *), long);
static time_t	time2(struct tm *, void (*)(const time_t *, long, struct tm *), long, int *);
static int		tmcomp(const struct tm *, const struct tm *);
static time_t	transtime(time_t, int, const struct rule *, long);

char *
ctime(t)
	const time_t *t;
{
	return (asctime(localtime(t)));
}

char *
ctime_r(t, buf)
	const time_t *t;
	char *buf;
{
	struct tm tmp;

	return (asctime_r(localtime_r(t, &tmp), buf));
}

char *
asctime_r(timeptr, buf)
	register const struct tm *timeptr;
	char *buf;
{
	static const char	wday_name[DAYS_PER_WEEK][3] = {
			"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char	mon_name[MONS_PER_YEAR][3] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char year[INT_STRLEN_MAXIMUM(int) + 2];
	static char result[MAX_ASCTIME_BUF_SIZE];
	const char *wn;
	const char *mn;

	if (timeptr == NULL) {
		errno = EINVAL;
		return strcpy(buf, "??? ??? ?? ??:??:?? ????\n");
	}
	if (timeptr->tm_wday < 0 || timeptr->tm_wday >= DAYS_PER_WEEK)
		wn = "???";
	else
		wn = wday_name[timeptr->tm_wday];
	if (timeptr->tm_mon < 0 || timeptr->tm_mon >= MONS_PER_YEAR)
		mn = "???";
	else
		mn = mon_name[timeptr->tm_mon];
	/*
	** Use strftime's %Y to generate the year, to avoid overflow problems
	** when computing timeptr->tm_year + TM_YEAR_BASE.
	** Assume that strftime is unaffected by other out-of-range members
	** (e.g., timeptr->tm_mday) when processing "%Y".
	*/
   	strftime(year, sizeof(year), "%Y", timeptr);
	(void) snprintf(result, sizeof(result),
			((strlen(year) <= 4) ? ASCTIME_FMT : ASCTIME_FMT_B),
			wn,
			mn,
			timeptr->tm_mday, timeptr->tm_hour,
			timeptr->tm_min, timeptr->tm_sec,
			TM_YEAR_BASE + timeptr->tm_year);
	if (strlen(result) < STD_ASCTIME_BUF_SIZE || buf == buf_asctime) {
		memcpy(buf, result, sizeof(result));
	}
	return result;
}

/*
** A la X3J11
*/

char *
asctime(timeptr)
	register const struct tm *timeptr;
{
    return asctime_r(timeptr, buf_asctime);
}

static long
detzcode(codep)
	const char * const codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

static int
tzload1(name)
	register const char *name;
{
	register int	i;
	register int	fid;
	int	ttisstdcnt;

	if (name == 0 && (name = TZDEFAULT) == 0)
		return -1;
	{
		register const char *p;
		register int doaccess;
		char fullname[MAXPATHLEN];

		doaccess = name[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == 0)
				return -1;
			if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
				return -1;
			(void) strcpy(fullname, p);
			(void) strcat(fullname, "/");
			(void) strcat(fullname, name);
			/*
			 ** Set doaccess if '.' (as in "../") shows up in name.
			 */
			while (*name != '\0')
				if (*name++ == '.')
					doaccess = TRUE;
			name = fullname;
		}
		if (doaccess && access(name, 4) != 0)
			return -1;
		if ((fid = open(name, 0)) == -1)
			return -1;
	}
	{
		register const char *p;
		register struct tzhead *tzhp;
		char buf[sizeof(s) + sizeof(*tzhp)];

		i = read(fid, buf, sizeof(buf));
		if (close(fid) != 0 || i < sizeof(*tzhp))
			return -1;
		tzhp = (struct tzhead*) buf;
		ttisstdcnt = (int) detzcode(tzhp->tzh_ttisstdcnt);
		s.leapcnt = (int) detzcode(tzhp->tzh_leapcnt);
		s.timecnt = (int) detzcode(tzhp->tzh_timecnt);
		s.typecnt = (int) detzcode(tzhp->tzh_typecnt);
		s.charcnt = (int) detzcode(tzhp->tzh_charcnt);
		if (s.leapcnt < 0 || s.leapcnt > TZ_MAX_LEAPS || s.typecnt <= 0
				|| s.typecnt > TZ_MAX_TYPES || s.timecnt < 0
				|| s.timecnt > TZ_MAX_TIMES ||s.charcnt < 0
				|| s.charcnt > TZ_MAX_CHARS
				|| (ttisstdcnt != s.typecnt && ttisstdcnt != 0)) {
			return -1;
		}
		if (i
				< sizeof *tzhp + s.timecnt * (4 + sizeof(char))
						+ s.typecnt * (4 + 2 * sizeof(char))
						+ s.charcnt * sizeof(char) + s.leapcnt * 2 * 4
						+ ttisstdcnt * sizeof(char)) {
			return -1;
		}
		p = buf + sizeof *tzhp;
		for (i = 0; i < s.timecnt; ++i) {
			s.ats[i] = detzcode(p);
			p += 4;
		}
		for (i = 0; i < s.timecnt; ++i) {
			s.types[i] = (unsigned char) *p++;
		}
		for (i = 0; i < s.typecnt; ++i) {
			register struct ttinfo *ttisp;

			ttisp = &s.ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			if (ttisp->tt_isdst != 0 && ttisp->tt_isdst != 1)
				return -1;
			ttisp->tt_abbrind = (unsigned char) *p++;
		}
		for (i = 0; i < s.charcnt; ++i)
			s.chars[i] = *p++;
		s.chars[i] = '\0'; /* ensure '\0' at end */

		for (i = 0; i < s.leapcnt; ++i) {
			register struct lsinfo *	lsisp;

			lsisp = &s.lsis[i];
			lsisp->ls_trans = detzcode(p);
			p += 4;
			lsisp->ls_corr = detzcode(p);
			p += 4;
		}
		for (i = 1; i < s.typecnt; ++i) {
			register struct ttinfo *ttisp;

			ttisp = &s.ttis[i];
			if (ttisstdcnt == 0) {
				ttisp->tt_ttisstd = FALSE;
			} else {
				ttisp->tt_ttisstd = *p++;
				if (ttisp->tt_ttisstd != TRUE && ttisp->tt_ttisstd != FALSE)
					return -1;
			}
		}
	}
	/*
	 ** Check that all the local time type indices are valid.
	 */
	for (i = 0; i < s.timecnt; ++i)
		if (s.types[i] >= s.typecnt)
			return -1;
	/*
	 ** Check that all abbreviation indices are valid.
	 */
	for (i = 0; i < s.typecnt; ++i)
		if (s.ttis[i].tt_abbrind >= s.charcnt)
			return -1;

	/*
	 ** Set tzname elements to initial values.
	 */
	tzname[0] = tzname[1] = &s.chars[0];
#ifdef USG_COMPAT
	tzone = s.ttis[0].tt_gmtoff;
	daylight = 0;
#endif /* USG_COMPAT */
#ifdef ALTZONE
	altzone = 0
#endif /* ALTZONE */
	for (i = 1; i < s.typecnt; ++i) {
		register struct ttinfo *ttisp;

		ttisp = &s.ttis[i];
		if (ttisp->tt_isdst) {
			tzname[1] = &s.chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
			daylight = 1;
#endif /* USG_COMPAT */
		} else {
			tzname[0] = &s.chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
			tzone = ttisp->tt_gmtoff;
#endif /* USG_COMPAT */
#ifdef ALTZONE
			altzone = ttisp->tt_gmtoff;
#endif /* ALTZONE */
		}
	}
	return 0;
}

static int
tzload(name, sp)
	const char *name;
	struct state *sp;
{
	int load_result;

	load_result = 0;
	if ((sp != NULL) || (sp == &s)) {
		load_result = tzload1(name);
		if (load_result != 0) {
			sp->leapcnt = 0; /* so, we're off a little */
		}
	}
	return (load_result);
}

static int	mon_lengths[2][MONS_PER_YEAR] = {
		{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static int	year_lengths[2] = {
		DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

/*
** Given a pointer into a time zone string, scan until a character that is not
** a valid character in a zone name is found.  Return a pointer to that
** character.
*/

static const char *
getzname(strp)
	register const char *strp;
{
	register char	c;

	while ((c = *strp) != '\0' && !isdigit(c) && c != ',' && c != '-'
			&& c != '+')
		++strp;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a number from that string.
** Check that the number is within a specified range; if it is not, return
** NULL.
** Otherwise, return a pointer to the first character not part of the number.
*/

static const char *
getnum(strp, nump, min, max)
	register const char *strp;
	int * const nump;
	const int min;
	const int max;
{
	register char	c;
	register int	num;

	if (strp == NULL || !isdigit(*strp))
		return NULL;
	num = 0;
	while ((c = *strp) != '\0' && isdigit(c)) {
		num = num * 10 + (c - '0');
		if (num > max)
			return NULL; /* illegal value */
		++strp;
	}
	if (num < min)
		return NULL; /* illegal value */
	*nump = num;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a number of seconds,
** in hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the number
** of seconds.
*/

static const char *
getsecs(strp, secsp)
	register const char *strp;
	long * const secsp;
{
	int	num;

	strp = getnum(strp, &num, 0, HOURS_PER_DAY);
	if (strp == NULL)
		return NULL;
	*secsp = num * SECS_PER_HOUR;
	if (*strp == ':') {
		++strp;
		strp = getnum(strp, &num, 0, MINS_PER_HOUR - 1);
		if (strp == NULL)
			return NULL;
		*secsp += num * SECS_PER_MIN;
		if (*strp == ':') {
			++strp;
			strp = getnum(strp, &num, 0, SECS_PER_MIN - 1);
			if (strp == NULL)
				return NULL;
			*secsp += num;
		}
	}
	return strp;
}

/*
** Given a pointer into a time zone string, extract an offset, in
** [+-]hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the time.
*/

static const char *
getoffset(strp, offsetp)
	register const char *strp;
	long * const offsetp;
{
	register int	neg;

	if (*strp == '-') {
		neg = 1;
		++strp;
	} else if (isdigit(*strp) || *strp++ == '+')
		neg = 0;
	else
		return NULL; /* illegal offset */
	strp = getsecs(strp, offsetp);
	if (strp == NULL)
		return NULL; /* illegal time */
	if (neg)
		*offsetp = -*offsetp;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a rule in the form
** date[/time].  See POSIX section 8 for the format of "date" and "time".
** If a valid rule is not found, return NULL.
** Otherwise, return a pointer to the first character not part of the rule.
*/

static const char *
getrule(strp, rulep)
	const char *strp;
	register struct rule *const rulep;
{
	if (*strp == 'J') {
		/*
		 ** Julian day.
		 */
		rulep->r_type = JULIAN_DAY;
		++strp;
		strp = getnum(strp, &rulep->r_day, 1, DAYS_PER_NYEAR);
	} else if (*strp == 'M') {
		/*
		 ** Month, week, day.
		 */
		rulep->r_type = MONTH_NTH_DAY_OF_WEEK;
		++strp;
		strp = getnum(strp, &rulep->r_mon, 1, MONS_PER_YEAR);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_week, 1, 5);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_day, 0, DAYS_PER_WEEK - 1);
	} else if (isdigit(*strp)) {
		/*
		 ** Day of year.
		 */
		rulep->r_type = DAY_OF_YEAR;
		strp = getnum(strp, &rulep->r_day, 0, DAYS_PER_LYEAR - 1);
	} else
		return NULL; /* invalid format */
	if (strp == NULL)
		return NULL;
	if (*strp == '/') {
		/*
		 ** Time specified.
		 */
		++strp;
		strp = getsecs(strp, &rulep->r_time);
	} else
		rulep->r_time = 2 * SECS_PER_HOUR; /* default = 2:00:00 */
	return strp;
}

/*
** Given the Epoch-relative time of January 1, 00:00:00 GMT, in a year, the
** year, a rule, and the offset from GMT at the time that rule takes effect,
** calculate the Epoch-relative time that rule takes effect.
*/

static time_t
transtime(janfirst, year, rulep, offset)
	const time_t janfirst;
	const int year;
	register const struct rule *const rulep;
	const long offset;
{
	register int	leapyear;
	register time_t	value;
	register int	i;
	int		d, m1, yy0, yy1, yy2, dow;

    value = NULL;
	leapyear = isleap(year);
	switch (rulep->r_type) {

	case JULIAN_DAY:
		/*
		 ** Jn - Julian day, 1 == January 1, 60 == March 1 even in leap
		 ** years.
		 ** In non-leap years, or if the day number is 59 or less, just
		 ** add SECSPERDAY times the day number-1 to the time of
		 ** January 1, midnight, to get the day.
		 */
		value = janfirst + (rulep->r_day - 1) * SECS_PER_DAY;
		if (leapyear && rulep->r_day >= 60)
			value += SECS_PER_DAY;
		break;

	case DAY_OF_YEAR:
		/*
		 ** n - day of year.
		 ** Just add SECSPERDAY times the day number to the time of
		 ** January 1, midnight, to get the day.
		 */
		value = janfirst + rulep->r_day * SECS_PER_DAY;
		break;

	case MONTH_NTH_DAY_OF_WEEK:
		/*
		 ** Mm.n.d - nth "dth day" of month m.
		 */
		value = janfirst;
		for (i = 0; i < rulep->r_mon - 1; ++i)
			value += mon_lengths[leapyear][i] * SECS_PER_DAY;

		/*
		 ** Use Zeller's Congruence to get day-of-week of first day of
		 ** month.
		 */
		m1 = (rulep->r_mon + 9) % 12 + 1;
		yy0 = (rulep->r_mon <= 2) ? (year - 1) : year;
		yy1 = yy0 / 100;
		yy2 = yy0 % 100;
		dow = ((26 * m1 - 2) / 10 + 1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
		if (dow < 0)
			dow += DAYS_PER_WEEK;

		/*
		 ** "dow" is the day-of-week of the first day of the month.  Get
		 ** the day-of-month (zero-origin) of the first "dow" day of the
		 ** month.
		 */
		d = rulep->r_day - dow;
		if (d < 0)
			d += DAYS_PER_WEEK;
		for (i = 1; i < rulep->r_week; ++i) {
			if (d + DAYS_PER_WEEK >= mon_lengths[leapyear][rulep->r_mon - 1])
				break;
			d += DAYS_PER_WEEK;
		}

		/*
		 ** "d" is the day-of-month (zero-origin) of the day we want.
		 */
		value += d * SECS_PER_DAY;
		break;
	}

	/*
	 ** "value" is the Epoch-relative time of 00:00:00 GMT on the day in
	 ** question.  To get the Epoch-relative time of the specified local
	 ** time on that day, add the transition time and the current offset
	 ** from GMT.
	 */
	return value + rulep->r_time + offset;
}

static int
tzparse(name, sp, lastditch)
	const char *name;
	struct state *sp;
	const int lastditch;
{
	const char *stdname;
	const char *dstname;
	int stdlen;
	int dstlen;
	long stdoffset;
	long dstoffset;
	register time_t *atp;
	register unsigned char *typep;
	register char *cp;
	register int load_result;

	stdname = name;
	if (lastditch) {
		stdlen = strlen(name); /* length of standard zone name */
		name += stdlen;
		if (stdlen >= sizeof sp->chars)
			stdlen = (sizeof sp->chars) - 1;
	} else {
		name = getzname(name);
		stdlen = name - stdname;
		if (stdlen < 3)
			return -1;
	}
	if (*name == '\0') {
		return -1;
	} else {
		name = getoffset(name, &stdoffset);
		if (name == NULL) {
			return -1;
		}
	}
	load_result = tzload(TZDEFRULES, sp);
	if (*name != '\0') {
		dstname = name;
		name = getzname(name);
		dstlen = name - dstname; /* length of DST zone name */
		if (dstlen < 3)
			return -1;
		if (*name != '\0' && *name != ',' && *name != ';') {
			name = getoffset(name, &dstoffset);
			if (name == NULL)
				return -1;
		} else
			dstoffset = stdoffset - SECS_PER_HOUR;
		if (*name == ',' || *name == ';') {
			struct rule start;
			struct rule end;
			register int year;
			register time_t janfirst;
			time_t starttime;
			time_t endtime;

			++name;
			if ((name = getrule(name, &start)) == NULL)
				return -1;
			if (*name++ != ',')
				return -1;
			if ((name = getrule(name, &end)) == NULL)
				return -1;
			if (*name != '\0')
				return -1;
			sp->typecnt = 2; /* standard time and DST */
			/*
			 ** Two transitions per year, from EPOCH_YEAR to 2037.
			 */
			sp->timecnt = 2 * (2037 - EPOCH_YEAR + 1);
			if (sp->timecnt > TZ_MAX_TIMES)
				return -1;
			sp->ttis[0].tt_gmtoff = -dstoffset;
			sp->ttis[0].tt_isdst = 1;
			sp->ttis[0].tt_abbrind = stdlen + 1;
			sp->ttis[1].tt_gmtoff = -stdoffset;
			sp->ttis[1].tt_isdst = 0;
			sp->ttis[1].tt_abbrind = 0;
			atp = sp->ats;
			typep = sp->types;
			janfirst = 0;
			for (year = EPOCH_YEAR; year <= 2037; ++year) {
				starttime = transtime(janfirst, year, &start, stdoffset);
				endtime = transtime(janfirst, year, &end, dstoffset);
				if (starttime > endtime) {
					*atp++ = endtime;
					*typep++ = 1; /* DST ends */
					*atp++ = starttime;
					*typep++ = 0; /* DST begins */
				} else {
					*atp++ = starttime;
					*typep++ = 0; /* DST begins */
					*atp++ = endtime;
					*typep++ = 1; /* DST ends */
				}
				janfirst += year_lengths[isleap(year)] * SECS_PER_DAY;
			}
		} else {
			int sawstd;
			int sawdst;
			long stdfix;
			long dstfix;
			long oldfix;
			int isdst;
			register int i;

			if (*name != '\0')
				return -1;
			if (load_result != 0)
				return -1;
			/*
			 ** Compute the difference between the real and
			 ** prototype standard and summer time offsets
			 ** from GMT, and put the real standard and summer
			 ** time offsets into the rules in place of the
			 ** prototype offsets.
			 */
			sawstd = FALSE;
			sawdst = FALSE;
			stdfix = 0;
			dstfix = 0;
			for (i = 0; i < sp->typecnt; ++i) {
				if (sp->ttis[i].tt_isdst) {
					oldfix = dstfix;
					dstfix = sp->ttis[i].tt_gmtoff + dstoffset;
					if (sawdst && (oldfix != dstfix))
						return -1;
					sp->ttis[i].tt_gmtoff = -dstoffset;
					sp->ttis[i].tt_abbrind = stdlen + 1;
					sawdst = TRUE;
				} else {
					oldfix = stdfix;
					stdfix = sp->ttis[i].tt_gmtoff + stdoffset;
					if (sawstd && (oldfix != stdfix))
						return -1;
					sp->ttis[i].tt_gmtoff = -stdoffset;
					sp->ttis[i].tt_abbrind = 0;
					sawstd = TRUE;
				}
			}
			/*
			 ** Make sure we have both standard and summer time.
			 */
			if (!sawdst || !sawstd)
				return -1;
			/*
			 ** Now correct the transition times by shifting
			 ** them by the difference between the real and
			 ** prototype offsets.  Note that this difference
			 ** can be different in standard and summer time;
			 ** the prototype probably has a 1-hour difference
			 ** between standard and summer time, but a different
			 ** difference can be specified in TZ.
			 */
			isdst = FALSE; /* we start in standard time */
			for (i = 0; i < sp->timecnt; ++i) {
				register const struct ttinfo *ttisp;

				/*
				 ** If summer time is in effect, and the
				 ** transition time was not specified as
				 ** standard time, add the summer time
				 ** offset to the transition time;
				 ** otherwise, add the standard time offset
				 ** to the transition time.
				 */
				ttisp = &sp->ttis[sp->types[i]];
				sp->ats[i] += (isdst && !ttisp->tt_ttisstd) ? dstfix : stdfix;
				isdst = ttisp->tt_isdst;
			}
		}
	} else {
		dstlen = 0;
		sp->typecnt = 1; /* only standard time */
		sp->timecnt = 0;
		sp->ttis[0].tt_gmtoff = -stdoffset;
		sp->ttis[0].tt_isdst = 0;
		sp->ttis[0].tt_abbrind = 0;
	}
	sp->charcnt = stdlen + 1;
	if (dstlen != 0)
		sp->charcnt += dstlen + 1;
	if (sp->charcnt > sizeof sp->chars)
		return -1;
	cp = sp->chars;
	(void) strncpy(cp, stdname, stdlen);
	cp += stdlen;
	*cp++ = '\0';
	if (dstlen != 0) {
		(void) strncpy(cp, dstname, dstlen);
		*(cp + dstlen) = '\0';
	}
	return 0;
}

static int
tzsetkernel(void)
{
	struct timeval	tv;
	struct timezone	tz;

	if (gettimeofday(&tv, &tz))
		return -1;
	s.timecnt = 0; /* UNIX counts *west* of Greenwich */
	s.leapcnt = 0;
	s.ttis[0].tt_gmtoff = tz.tz_minuteswest * -SECS_PER_MIN;
	s.ttis[0].tt_abbrind = 0;
	(void) strcpy(s.chars, tztab(tz.tz_minuteswest, 0));
	tzname[0] = tzname[1] = s.chars;
#ifdef USG_COMPAT
	tzone = tz.tz_minuteswest * 60;
	daylight = tz.tz_dsttime;
#endif /* USG_COMPAT */
#ifdef ALTZONE
	altzone = tz.tz_minuteswest * 60;
#endif /* ALTZONE */
	return 0;
}

static void
tzsetgmt(void)
{
	s.timecnt = 0;
	s.leapcnt = 0;
	s.ttis[0].tt_gmtoff = 0;
	s.ttis[0].tt_abbrind = 0;
	(void) strcpy(s.chars, GMT);
	tzname[0] = tzname[1] = s.chars;
#ifdef USG_COMPAT
	tzone = 0;
	daylight = 0;
#endif /* USG_COMPAT */
#ifdef ALTZONE
	altzone = 0;
#endif /* ALTZONE */
}

#ifdef notyet
void
tzset(void)
{
	register char *name;

	lcl_is_set = TRUE;
	name = getenv("TZ");
	if (!name || *name) { /* did not request GMT */
		if (name && !tzload(name)) /* requested name worked */
			return;
		if (!tzload((char*) 0)) /* default name worked */
			return;
		if (!tzsetkernel()) /* kernel guess worked */
			return;
	}
	tzsetgmt(); /* GMT is default */
}
#endif

void
tzset(void)
{
	register const char *name;

	lcl_is_set = TRUE;
	name = getenv("TZ");
	if (!name || *name) { /* did not request GMT */
		if (name && !tzload(name, lclptr)) { /* requested name worked */
			gmtload(lclptr);
			return;
		}
		if (!tzload((char*) 0, lclptr)) { /* default name worked */
			gmtload(lclptr);
			return;
		}
		if (!tzsetkernel()) { /* kernel guess worked */
			gmtload(lclptr);
			return;
		}
		if (name[0] == ':' || !tzparse(name, lclptr, FALSE)) {
			gmtload(lclptr);
		}
	}
	tzsetgmt(); /* GMT is default */
}

static void
gmtload(struct state *sp)
{
	if (tzload(GMT, sp) != 0) {
		(void) tzparse(GMT, sp, TRUE);
	}
}

static void
gmtcheck(void)
{
	 if (!gmt_is_set) {
		 gmtptr = (struct state *)malloc(sizeof *gmtptr);
	 }
	 if (gmtptr) {
		 gmtload(gmtptr);
	 }
	 gmt_is_set = TRUE;

#ifdef ALL_STATE
	if (!lcl_is_set) {
		lclptr = (struct state *)malloc(sizeof *lclptr);
	}
	if (lclptr) {
		gmtload(lclptr);
	}
	lcl_is_set = TRUE;
#endif
}

static struct tm *
localtimesub(timep, tmp)
	const time_t *timep;
	struct tm *tmp;
{
	register struct ttinfo *ttisp;
	register int i;
	time_t t;

	if (!lcl_is_set)
		(void)tzset();
	t = *timep;
	if (s.timecnt == 0 || t < s.ats[0]) {
		i = 0;
		while (s.ttis[i].tt_isdst)
			if (++i >= s.timecnt) {
				i = 0;
				break;
			}
	} else {
		for (i = 1; i < s.timecnt; ++i)
			if (t < s.ats[i])
				break;
		i = s.types[i - 1];
	}
	ttisp = &s.ttis[i];
	/*
	 ** To get (wrong) behavior that's compatible with System V Release 2.0
	 ** you'd replace the statement below with
	 **	tmp = offtime((time_t) (t + ttisp->tt_gmtoff), 0L);
	 */
	tmp = offtime_r(&t, ttisp->tt_gmtoff, tmp);
	tmp->tm_isdst = ttisp->tt_isdst;
	tzname[tmp->tm_isdst] = &s.chars[ttisp->tt_abbrind];
	tmp->tm_zone = &s.chars[ttisp->tt_abbrind];
	return tmp;
}

struct tm *
localtime_r(timep, tmp)
	const time_t *timep;
	struct tm *tmp;
{
	return localtimesub(timep, tmp);
}

struct tm *
localtime(timep)
	const time_t *timep;
{
	struct tm tmp;

	return localtime_r(timep, &tmp);
}

static struct tm *
gmttimesub(clock, offset, tmp)
	const time_t *clock;
	long		offset;
	struct tm 	*tmp;
{
	register struct tm *result;

	result = offtime_r(clock, offset, tmp);
	if (result) {
		tzname[0] = GMT;
		result->tm_zone = __UNCONST(offset ? wildabbr : gmtptr ? gmtptr->chars : GMT); /* UCT ? */
	}
	return result;
}

struct tm *
gmtime_r(clock, tmp)
	const time_t *clock;
	struct tm *tmp;
{
	gmtcheck();
	return gmttimesub(clock, 0L, tmp);
}

struct tm *
gmtime(clock)
	const time_t *clock;
{
	struct tm tmp;

	return gmtime_r(clock, &tmp);
}

struct tm *
offtime(clock, offset)
	const time_t *clock;
	long		offset;
{
	register struct tm 	*tmp;
	static struct tm tm;

	tmp = &tm;
	gmtcheck();
	return (offtime_r(clock, offset, tmp));
}

struct tm *
offtime_r(clock, offset, tmp)
	const time_t *clock;
	long		offset;
	struct tm 	*tmp;
{
	register long		days;
	register long		rem;
	register int		y;
	register int		yleap;
	register int        *ip;

	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	rem += offset;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = (int) (rem / SECS_PER_MIN);
	tmp->tm_sec = (int) (rem % SECS_PER_MIN);
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYS_PER_WEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for (;;) {
			yleap = isleap(y);
			if (days < (long) year_lengths[yleap])
				break;
			++y;
			days = days - (long) year_lengths[yleap];
		}
	else
		do {
			--y;
			yleap = isleap(y);
			days = days + (long) year_lengths[yleap];
		} while (days < 0);
	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;
	ip = mon_lengths[yleap];
	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days = days - (long) ip[tmp->tm_mon];
	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
	tmp->tm_zone = __UNCONST("");
	tmp->tm_gmtoff = offset;
	return tmp;
}

/*
** The easy way to behave "as if no library function calls" localtime
** is to not call it--so we drop its guts into "localsub", which can be
** freely called.  (And no, the PANS doesn't require the above behavior--
** but it *is* desirable.)
**
** The unused offset argument is for the benefit of mktime variants.
*/
static void
localsub(timep, offset, tmp)
	const time_t *timep;
	long offset;
	struct tm *tmp;
{
	register struct state *sp;
	struct tm *result;

	if (!lcl_is_set) {
		(void)tzset();
	}
	sp = lclptr;
#ifdef ALL_STATE
	if (sp == NULL) {
		result = gmttimesub(timep, offset, tmp);
		if (result != NULL) {
			tmp = result;
		}
		return;
	}
#endif /* defined ALL_STATE */
	result = localtimesub(timep, tmp);
	if (result != NULL) {
		tmp = result;
	}
}

static void
gmtsub(timep, offset, tmp)
	const time_t *timep;
	long offset;
	struct tm *tmp;
{
	struct tm *result;

	if (!lcl_is_set) {
		(void)tzset();
	}
	result = gmttimesub(timep, offset, tmp);
	if (result != NULL) {
		tmp = result;
	}
}

/*
 ** Adapted from code provided by Robert Elz, who writes:
 **	The "best" way to do mktime I think is based on an idea of Bob
 **	Kridle's (so its said...) from a long time ago. (mtxinu!kridle now).
 **	It does a binary search of the time_t space.  Since time_t's are
 **	just 32 bits, its a max of 32 iterations (even at 64 bits it
 **	would still be very reasonable).
 */

#ifndef WRONG
#define WRONG	(-1)
#endif /* !defined WRONG */

static const struct state *
state_alloc(funcp)
	void (*funcp)(const time_t *, long, struct tm *);
{
	register const struct state *sp;

	if (funcp == localsub) {
		sp = lclptr;
	} else {
		sp = gmtptr;
	}
#ifdef ALL_STATE
	if (sp == NULL) {
		return NULL;
	}
#endif
	return sp;
}

static void
normalize(tensptr, unitsptr, base)
	int * const	tensptr;
	int * const	unitsptr;
	const int	base;
{
	if (*unitsptr >= base) {
		*tensptr += *unitsptr / base;
		*unitsptr %= base;
	} else if (*unitsptr < 0) {
		*tensptr -= 1 + (-(*unitsptr + 1)) / base;
		*unitsptr = base - 1 - (-(*unitsptr + 1)) % base;
	}
}

static int
tmcomp(atmp, btmp)
	register const struct tm * const atmp;
	register const struct tm * const btmp;
{
	register int result;

	if ((result = (atmp->tm_year - btmp->tm_year)) == 0
			&& (result = (atmp->tm_mon - btmp->tm_mon)) == 0
			&& (result = (atmp->tm_mday - btmp->tm_mday)) == 0
			&& (result = (atmp->tm_hour - btmp->tm_hour)) == 0
			&& (result = (atmp->tm_min - btmp->tm_min)) == 0)
		result = atmp->tm_sec - btmp->tm_sec;
	return result;
}

static time_t
time2(tmp, funcp, offset, okayp)
	struct tm *const tmp;
	void (*funcp)(const time_t *, long, struct tm *);
	const long offset;
	int *const okayp;
{
	register const struct state *sp;
	register int dir;
	register int bits;
	register int i, j;
	register int saved_seconds;
	time_t newt;
	time_t t;
	struct tm yourtm, mytm;

	*okayp = FALSE;
	yourtm = *tmp;
	if (yourtm.tm_sec >= SECS_PER_MIN + 2 || yourtm.tm_sec < 0)
		normalize(&yourtm.tm_min, &yourtm.tm_sec, SECS_PER_MIN);
	normalize(&yourtm.tm_hour, &yourtm.tm_min, MINS_PER_HOUR);
	normalize(&yourtm.tm_mday, &yourtm.tm_hour, HOURS_PER_DAY);
	normalize(&yourtm.tm_year, &yourtm.tm_mon, MONS_PER_YEAR);
	while (yourtm.tm_mday <= 0) {
		--yourtm.tm_year;
		yourtm.tm_mday += year_lengths[isleap(yourtm.tm_year + TM_YEAR_BASE)];
	}
	while (yourtm.tm_mday > DAYS_PER_LYEAR) {
		yourtm.tm_mday -= year_lengths[isleap(yourtm.tm_year + TM_YEAR_BASE)];
		++yourtm.tm_year;
	}
	for (;;) {
		i = mon_lengths[isleap(yourtm.tm_year + TM_YEAR_BASE)][yourtm.tm_mon];
		if (yourtm.tm_mday <= i)
			break;
		yourtm.tm_mday -= i;
		if (++yourtm.tm_mon >= MONS_PER_YEAR) {
			yourtm.tm_mon = 0;
			++yourtm.tm_year;
		}
	}
	saved_seconds = yourtm.tm_sec;
	yourtm.tm_sec = 0;
	/*
	** Calculate the number of magnitude bits in a time_t
	** (this works regardless of whether time_t is
	** signed or unsigned, though lint complains if unsigned).
	*/
	for (bits = 0, t = 1; t > 0; ++bits, t <<= 1)
		;
	/*
	** If time_t is signed, then 0 is the median value,
	** if time_t is unsigned, then 1 << bits is median.
	*/
	t = (t < 0) ? 0 : ((time_t) 1 << bits);
	for ( ; ; ) {
		(*funcp)(&t, offset, &mytm);
		dir = tmcomp(&mytm, &yourtm);
		if (dir != 0) {
			if (bits-- < 0)
				return WRONG;
			if (bits < 0)
				--t;
			else if (dir > 0)
				t -= (time_t) 1 << bits;
			else	t += (time_t) 1 << bits;
			continue;
		}
		if (yourtm.tm_isdst < 0 || mytm.tm_isdst == yourtm.tm_isdst)
			break;
		/*
		** Right time, wrong type.
		** Hunt for right time, right type.
		** It's okay to guess wrong since the guess
		** gets checked.
		*/
		sp = state_alloc(funcp);
		for (i = 0; i < sp->typecnt; ++i) {
			if (sp->ttis[i].tt_isdst != yourtm.tm_isdst)
				continue;
			for (j = 0; j < sp->typecnt; ++j) {
				if (sp->ttis[j].tt_isdst == yourtm.tm_isdst)
					continue;
				newt = t + sp->ttis[j].tt_gmtoff -
					sp->ttis[i].tt_gmtoff;
				(*funcp)(&newt, offset, &mytm);
				if (tmcomp(&mytm, &yourtm) != 0)
					continue;
				if (mytm.tm_isdst != yourtm.tm_isdst)
					continue;
				/*
				** We have a match.
				*/
				t = newt;
				goto label;
			}
		}
		return WRONG;
	}
label:
	t += saved_seconds;
	(*funcp)(&t, offset, tmp);
	*okayp = TRUE;
	return t;
}

static time_t
time1(tmp, funcp, offset)
	struct tm * const tmp;
	void (*funcp)(const time_t *, long, struct tm *);
	const long	offset;
{
	register time_t	t;
	register const struct state *sp;
	register int samei, otheri;
	int okay;

	if (tmp->tm_isdst > 1)
		tmp->tm_isdst = 1;
	t = time2(tmp, funcp, offset, &okay);
	if (okay || tmp->tm_isdst < 0)
		return t;
	/*
	** We're supposed to assume that somebody took a time of one type
	** and did some math on it that yielded a "struct tm" that's bad.
	** We try to divine the type they started from and adjust to the
	** type they need.
	*/
	sp = state_alloc(funcp);
	for (samei = 0; samei < sp->typecnt; ++samei) {
		if (sp->ttis[samei].tt_isdst != tmp->tm_isdst)
			continue;
		for (otheri = 0; otheri < sp->typecnt; ++otheri) {
			if (sp->ttis[otheri].tt_isdst == tmp->tm_isdst)
				continue;
			tmp->tm_sec += sp->ttis[otheri].tt_gmtoff
					- sp->ttis[samei].tt_gmtoff;
			tmp->tm_isdst = !tmp->tm_isdst;
			t = time2(tmp, funcp, offset, &okay);
			if (okay)
				return t;
			tmp->tm_sec -= sp->ttis[otheri].tt_gmtoff
					- sp->ttis[samei].tt_gmtoff;
			tmp->tm_isdst = !tmp->tm_isdst;
		}
	}
	return WRONG;
}

time_t
mktime(tmp)
	struct tm * const tmp;
{
	return time1(tmp, localsub, 0L);
}

time_t
timegm(tmp)
	struct tm * const tmp;
{
	if (tmp != NULL) {
		tmp->tm_isdst = 0;
	}
	return time1(tmp, gmtsub, 0L);
}
