#	@(#)Makefile	5.8 (Berkeley) 3/10/91

WARNS= 5

#.include <bsd.own.mk>

PROG=   ar
SRCS=	append.c ar.c archive.c contents.c delete.c extract.c misc.c \
    	move.c print.c replace.c strmode.c

MAN=	ar.1 ar.5.5

.if (${HOSTPROG:U} == "")
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
.endif

.include <bsd.prog.mk>
