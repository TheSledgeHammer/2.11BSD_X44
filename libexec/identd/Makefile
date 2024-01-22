# $NetBSD: Makefile,v 1.11 2005/04/03 22:15:32 peter Exp $

.include <bsd.own.mk>

WARNS=	4
PROG=	identd
SRCS=	identd.c
MAN=	identd.8


# Build with pf support?
.if (${MKPF} != "no")
SRCS+=	pf.c
CPPFLAGS+=-DWITH_PF
.endif

.include <bsd.prog.mk>
