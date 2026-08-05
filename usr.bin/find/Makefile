#	$NetBSD: Makefile,v 1.9 2002/09/18 14:00:36 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	find
SRCS=	find.c function.c ls.c main.c misc.c operator.c option.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
