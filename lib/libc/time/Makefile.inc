#	$NetBSD: Makefile.inc,v 1.9 2001/03/31 19:00:56 kleink Exp $

.PATH: ${.CURDIR}/time

SRCS+=		ctime.c	difftime.c strftime.c strptime.c
MAN+=		ctime.3 strftime.3 strptime.3 time2posix.3 tzfile.5 tzset.3
CPPFLAGS+=	-DALL_STATE -DUSG_COMPAT

MLINKS+=	ctime.3 asctime.3		\
			ctime.3 difftime.3		\
			ctime.3 gmtime.3		\
			ctime.3 localtime.3		\
			ctime.3 mktime.3		\
			tzset.3 daylight.3		\
			tzset.3 tzsetwall.3
