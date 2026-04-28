#	$NetBSD: Makefile,v 1.23 2002/09/18 14:00:36 lukem Exp $

.include <bsd.own.mk>

PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c compat.c

CRONDIR= ${NETBSDSRCDIR}/usr.sbin/cron 
CPPFLAGS+=-I${.CURDIR} -I${CRONDIR} -DDEBUGGING=1
CPPFLAGS+=-DUSE_UTIMES -DPOSIX

BINOWN=root
BINMODE=4555
MAN= crontab.1 crontab.5

.PATH: ${CRONDIR}

COPTS.crontab.c+= ${GCC_NO_FORMAT_OVERFLOW}
COPTS.misc.c+= ${GCC_NO_FORMAT_OVERFLOW}

.include <bsd.prog.mk>
