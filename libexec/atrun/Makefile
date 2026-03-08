#	$NetBSD: Makefile,v 1.13 2002/09/18 06:24:33 lukem Exp $

#.include <bsd.own.mk>

PROG=	atrun
SRCS= 	atrun.c
BINDIR=	/usr/libexec
MAN=	at.1

AT_DIR= ${NETBSDSRCDIR}/usr.bin/at

CPPFLAGS+= -I. -I${.CURDIR} -I${AT_DIR}
.PATH: ${AT_DIR}

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

#COPTS.atrun.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
