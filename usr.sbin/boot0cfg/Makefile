# $FreeBSD$

.include <bsd.own.mk>

PROG=	boot0cfg
MAN=	boot0cfg.8

COPTS.boot0cfg.c+= ${GCC_NO_STRINGOP_OVERFLOW} ${GCC_NO_FORMAT_OVERFLOW}
#NO_WCAST_ALIGN=

.include <bsd.prog.mk>
