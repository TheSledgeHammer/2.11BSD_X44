#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.init.mk>

PROG=	date
SRCS=	date.c netdate.c
.if !defined(HOSTPROG)
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif
CPPFLAGS+=-I${.CURDIR}
CPPFLAGS+=-D__SELECT_DECLARED

COPTS.date.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
