# $FreeBSD$

.include <bsd.own.mk>

PROG=	btxld
MAN=	btxld.8
SRCS=	btxld.c elfh.c

.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

.include <bsd.prog.mk>
