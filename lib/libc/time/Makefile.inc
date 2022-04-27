#	$NetBSD: Makefile.inc,v 1.9 2001/03/31 19:00:56 kleink Exp $

.PATH: ${.CURDIR}/time

SRCS+=		ctime.c							\
MAN+=		ctime.3
CFLAGS+=	-DALL_STATE -DUSG_COMPAT

MLINKS+=	ctime.3 asctime.3 ctime.3 		\
			difftime.3 ctime.3 gmtime.3 	\
			ctime.3 localtime.3 ctime.3 	\
			mktime.3
MLINKS+=	tzset.3 tzsetwall.3
