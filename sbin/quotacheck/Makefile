#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	quotacheck
SRCS=	quotacheck.c preen.c fsutil.c
MAN=	quotacheck.8

FSCKSRC= ${NETBSDSRCDIR}/sbin/fsck
.PATH:  ${FSCKSRC}

CPPFLAGS+=-I${FSCKSRC}

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
