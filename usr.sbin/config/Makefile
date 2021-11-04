#	$NetBSD: Makefile,v 1.29 2003/10/19 23:15:54 he Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94
.include <bsd.own.mk>
WARNS=6

PROG=	config
SRCS=	files.c gram.y hash.c main.c mkdevsw.c mkheaders.c mkioconf.c \
		mkmakefile.c mkswap.c pack.c scan.l sem.c util.c
	
MAN=	config.8
YHEADER=1
CPPFLAGS+=-I${.CURDIR} -I.

.ifndef HOSTPROG
LDADD+=-lutil
DPADD+=${LIBUTIL}
COPTS.mkheaders.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.sem.c+=		${GCC_NO_FORMAT_TRUNCATION}
.endif

CWARNFLAGS+=-Wno-format-y2k

.include <bsd.prog.mk>
