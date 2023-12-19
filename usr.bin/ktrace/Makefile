#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	ktrace
SRCS=	ktrace.c subr.c
#MLINKS=	ktrace.1

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

.include <bsd.prog.mk>
