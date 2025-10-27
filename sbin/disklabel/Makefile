#	@(#)Makefile	8.2 (Berkeley) 3/17/94

PROG=	disklabel
SRCS=	bsdlabel.c dkcksum.c
MAN=	disklabel.5.5 disklabel.8

#.if (${HOSTPROG:U} == "")
#DPADD+= ${LIBUTIL}
#LDADD+= -lutil
#.endif

COPTS.bsdlabel.c+=	${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_FORMAT_OVERFLOW}

.include <bsd.prog.mk>
