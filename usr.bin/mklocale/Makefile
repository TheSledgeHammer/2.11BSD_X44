#	@(#)Makefile	8.1 (Berkeley) 6/7/93

.include <bsd.own.mk>

WARNS?=		2
PROG=		mklocale
SRCS=		yacc.c lex.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
YHEADER=	1

.include <bsd.prog.mk>
