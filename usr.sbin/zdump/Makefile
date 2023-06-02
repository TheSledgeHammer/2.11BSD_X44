#	$NetBSD: Makefile,v 1.5 2002/09/18 03:54:39 lukem Exp $

.include "${.PARSEDIR}/../zic/Makefile.inc"
.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c ialloc.c
MAN=	zdump.8

LDADD+= -lintl
DPADD+= ${LIBINTL}

COPTS.zdump.c += -Wno-format-nonliteral
CWARNFLAGS.clang+= -Wno-error=bitwise-conditional-parentheses

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>