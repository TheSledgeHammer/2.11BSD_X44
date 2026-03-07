#	$NetBSD: Makefile,v 1.12 2016/03/13 00:32:09 dholland Exp $

USE_FORT?=	yes	# setuid

PROG_AT= at
AT_SRCS= at.c

PROG_ATQ= atq
ATQ_SRCS= atq.c

PROG_ATRM= atrm
ATRM_SRCS= atrm.c

PROG= ${PROG_AT} ${PROG_ATQ} ${PROG_ATRM}
SRCS= ${AT_SRCS} ${ATQ_SRCS} ${ATRM_SRCS}

LINKS=	${BINDIR}/at ${BINDIR}/atq \
		${BINDIR}/at ${BINDIR}/atrm \
		${BINDIR}/at ${BINDIR}/batch
		
BINOWN=		root
BINMODE=	4555

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

COPTS.atq.c= ${GCC_NO_FORMAT_OVERFLOW}

.include <bsd.prog.mk>
