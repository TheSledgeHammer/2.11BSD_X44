#	$NetBSD: Makefile,v 1.12 2016/03/13 00:32:09 dholland Exp $

USE_FORT?=	yes	# setuid

PROG=		at
SRCS=		atq.c atrm.c at.c

LINKS=	${BINDIR}/at ${BINDIR}/atq \
		${BINDIR}/at ${BINDIR}/atrm \
		${BINDIR}/at ${BINDIR}/batch
		
BINOWN=		root
BINMODE=	4555

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

.include <bsd.prog.mk>