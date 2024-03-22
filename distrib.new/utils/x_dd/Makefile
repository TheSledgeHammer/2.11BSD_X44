# $NetBSD: Makefile,v 1.2 2001/12/12 00:05:12 tv Exp $

SRCDIR=		${.CURDIR}/../../../bin/dd

PROG=		dd
NOMAN=		# defined

SRCS=		args.c conv.c dd.c misc.c position.c
CPPFLAGS+=	-DNO_CONV -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
