#	$NetBSD: Makefile,v 1.10 2003/03/14 21:56:07 christos Exp $

PROG=	cron
SRCS=	cron.c database.c user.c entry.c compat.c \
		misc.c job.c do_command.c env.c popen.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	bitstring.3 cron.8

.include <bsd.prog.mk>