/*
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)ctime.c	1.1 (Berkeley) 3/25/87";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/param.h>

#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#include "private.h"


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
#define ASCTIME_FMT	"%.3s %.3s%3d %2.2d:%2.2d:%2.2d %-4s\n"
                      
/*
** For years that are more than four digits we put extra spaces before the year
** so that code trying to overwrite the newline won't end up overwriting
** a digit within a year and truncating the year (operating on the assumption
** that no output is better than wrong output).
*/
#define ASCTIME_FMT_B	"%.3s %.3s%3d %2.2d:%2.2d:%2.2d     %s\n"

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

struct ttinfo {					/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
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

const char *tzname[2] = {
		"GMT",
		"GMT"
};

#ifdef USG_COMPAT
time_t		tzone = 0;
int			daylight = 0;
#endif /* USG_COMPAT */

static struct state	s;
static int	tz_is_set;

static struct state *const gmtptr = &s;

//struct tm 	*offtime(const time_t *, long);
//static struct tm *offtime_r(const time_t *, long, struct tm *);

static long detzcode(const char *);
static int tzload(const char *);
static int tzsetkernel(void);
static void tzsetgmt(void);

static int tzparse(const char *, struct state *const);
static void gmtload(struct state *const);
static void gmtcheck(void);
static struct tm *localtimesub(const time_t *, const struct tm *);
static struct tm *gmtsub(const time_t *, long, struct tm *);

char *
ctime(t)
	const time_t *t;
{
	return (asctime(localtime(t)));
}

/*
** A la X3J11
*/

char *
asctime(timeptr)
	register const struct tm *timeptr;
{
	static const char	wday_name[DAYS_PER_WEEK][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char	mon_name[MONS_PER_YEAR][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
    static char	year[INT_STRLEN_MAXIMUM(int) + 2];
	static char	result[MAX_ASCTIME_BUF_SIZE];

	/*
	** Use strftime's %Y to generate the year, to avoid overflow problems
	** when computing timeptr->tm_year + TM_YEAR_BASE.
	** Assume that strftime is unaffected by other out-of-range members
	** (e.g., timeptr->tm_mday) when processing "%Y".
	*/
    strftime(year, sizeof(year), "%Y", timeptr);
    (void) snprintf(result, sizeof(result), 
        ((strlen(year) <= 4) ? ASCTIME_FMT : ASCTIME_FMT_B),
		wday_name[timeptr->tm_wday],
		mon_name[timeptr->tm_mon],
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		TM_YEAR_BASE + timeptr->tm_year);
	return result;
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
tzload(name)
	register const char *name;
{
	register int	i;
	register int	fid;

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
		char buf[sizeof s];

		i = read(fid, buf, sizeof buf);
		if (close(fid) != 0 || i < sizeof *tzhp)
			return -1;
		tzhp = (struct tzhead*) buf;
		s.timecnt = (int) detzcode(tzhp->tzh_timecnt);
		s.typecnt = (int) detzcode(tzhp->tzh_typecnt);
		s.charcnt = (int) detzcode(tzhp->tzh_charcnt);
		if (s.timecnt > TZ_MAX_TIMES || s.typecnt == 0||
		s.typecnt > TZ_MAX_TYPES ||
		s.charcnt > TZ_MAX_CHARS)
			return -1;
		if (i
				< sizeof *tzhp + s.timecnt * (4 + sizeof(char))
						+ s.typecnt * (4 + 2 * sizeof(char))
						+ s.charcnt * sizeof(char))
			return -1;
		p = buf + sizeof *tzhp;
		for (i = 0; i < s.timecnt; ++i) {
			s.ats[i] = detzcode(p);
			p += 4;
		}
		for (i = 0; i < s.timecnt; ++i)
			s.types[i] = (unsigned char) *p++;
		for (i = 0; i < s.typecnt; ++i) {
			register struct ttinfo *ttisp;

			ttisp = &s.ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			ttisp->tt_abbrind = (unsigned char) *p++;
		}
		for (i = 0; i < s.charcnt; ++i)
			s.chars[i] = *p++;
		s.chars[i] = '\0'; /* ensure '\0' at end */
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
		}
	}
	return 0;
}

static int
tzparse(name, sp)
	const char *name;
	struct state *const sp;
{
	if (sp != NULL) {
		if ((tzload(name) != 0) && (sp == &s)) {
			return (0);
		}
	}
	return (-1);
}

static int
tzsetkernel(void)
{
	struct timeval	tv;
	struct timezone	tz;

	if (gettimeofday(&tv, &tz))
		return -1;
	s.timecnt = 0; /* UNIX counts *west* of Greenwich */
	s.ttis[0].tt_gmtoff = tz.tz_minuteswest * -SECS_PER_MIN;
	s.ttis[0].tt_abbrind = 0;
	(void) strcpy(s.chars, tztab(tz.tz_minuteswest, 0));
	tzname[0] = tzname[1] = s.chars;
#ifdef USG_COMPAT
	tzone = tz.tz_minuteswest * 60;
	daylight = tz.tz_dsttime;
#endif /* USG_COMPAT */
	return 0;
}

static void
tzsetgmt(void)
{
	s.timecnt = 0;
	s.ttis[0].tt_gmtoff = 0;
	s.ttis[0].tt_abbrind = 0;
	(void) strcpy(s.chars, "GMT");
	tzname[0] = tzname[1] = s.chars;
#ifdef USG_COMPAT
	tzone = 0;
	daylight = 0;
#endif /* USG_COMPAT */
}

void
tzset(void)
{
	register char *name;

	tz_is_set = TRUE;
	name = getenv("TZ");
	if (!name || *name) {			/* did not request GMT */
		if (name && !tzload(name))	/* requested name worked */
			return;
		if (!tzload((char *)0))		/* default name worked */
			return;
		if (!tzsetkernel())		/* kernel guess worked */
			return;
	}
	tzsetgmt();				/* GMT is default */
}

static void
gmtload(struct state *const sp)
{
	if (tzload("GMT") != 0) {
		(void) tzparse("GMT", sp);
	}
}

static void
gmtcheck(void)
{
	 static bool gmt_is_set;

	 if (!gmt_is_set) {
		 gmtptr = malloc(sizeof *gmtptr);
	 }
	 if (gmtptr) {
		 gmtload(gmtptr);
	 }
	 gmt_is_set = true;
}

static struct tm *
localtimesub(timep, tmp)
	const time_t *timep;
	const struct tm *tmp;
{
	register struct ttinfo *ttisp;
	//register struct tm *tmp;
	register struct tm *result;
	register int i;
	time_t t;

	if (!tz_is_set)
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
	result = offtime_r(&t, ttisp->tt_gmtoff, tmp);
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
	register struct tm 	tmp;

	return localtime_r(timep, &tmp);
}

static struct tm *
gmtsub(clock, offset, tmp)
	const time_t *clock;
	long		offset;
	struct tm 	*tmp;
{
	register struct tm *result;

	result = offtime_r(clock, offset, tmp);
	if (result) {
		tzname[0] = "GMT";
		result->tm_zone = UNCONST(offset ? wildabbr : gmtptr ? gmtptr->chars : "GMT"); /* UCT ? */
	}
	return result;
}

struct tm *
gmtime_r(timep, tmp)
	time_t const *timep;
	struct tm *tmp;
{
	gmtcheck();
	return gmtsub(clock, 0L, tmp);
}

struct tm *
gmtime(clock)
	const time_t *clock;
{
	register struct tm tmp;

	return gmtime_r(clock, &tmp);
}

static int	mon_lengths[2][MONS_PER_YEAR] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static int	year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

struct tm *
offtime(clock, offset)
	const time_t *clock;
	long		offset;
{
	register struct tm 	*tmp;
	static struct tm tm;

	tmp = &tm;
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
