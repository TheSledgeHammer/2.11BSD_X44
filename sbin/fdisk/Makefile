# $FreeBSD$

PROG=	fdisk
SRCS=	fdisk.c
WARNS?=	4
MAN=	fdisk.8

COPTS.fdisk.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>

.if (${HOSTPROG:U} == "")
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif
