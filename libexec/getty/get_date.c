/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if	!defined(lint) && defined(DOSCCS)
#if 0
static char sccsid[] = "@(#)get_date.c	5.1.1 (2.11BSD GTE) 12/9/94";
#endif
#endif

#include <sys/time.h>

#include <stdio.h>
#include <string.h>

#include "extern.h"

static const char *days[] = {
	"Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"
};

static const char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "June",
	"July", "Aug", "Sept", "Oct", "Nov", "Dec"
};

#define AM "am"
#define PM "pm"

void
get_date(char *datebuffer)
{
	register struct tm *tmp;
	struct timeval tv;
	int realhour;
	register const char *zone;

	gettimeofday(&tv, 0);
	tmp = localtime(&tv.tv_sec);

	realhour = tmp->tm_hour;
	zone = AM;			/* default to morning */
	if (tmp->tm_hour == 0)
		realhour = 12;		/* midnight */
	else if (tmp->tm_hour == 12)
		zone = PM;		/* noon */
	else if (tmp->tm_hour >= 13 && tmp->tm_hour <= 23) { /* afternoon */
		realhour = realhour - 12;
		zone = PM;
	}
	
	/* format is '8:10pm on Sunday, 16 Sept 1973' */

	sprintf(datebuffer, "%d:%02d%s on %s, %d %s %d",
		realhour,
		tmp->tm_min,
		zone,
		days[tmp->tm_wday],
		tmp->tm_mday,
		months[tmp->tm_mon],
		1900 + tmp->tm_year);
}
