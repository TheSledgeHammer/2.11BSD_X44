#	$NetBSD: Makefile,v 1.13 2002/09/18 06:24:33 lukem Exp $

.include <bsd.own.mk>

PROG=	atrun
SRCS= 	atrun.c
BINDIR=	/usr/libexec
MAN=	atrun.8

AT_DIR:= ${NETBSDSRCDIR}/usr.bin/at

.PATH.c: ${AT_DIR}

CPPFLAGS+= -I${AT_DIR}
COPTS.atrun.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
