#	@(#)Makefile	8.1 (Berkeley) 6/7/93

.ifndef HOSTPROG
.include <bsd.own.mk>
.endif

WARNS?=		2
PROG=		mklocale
SRCS=		yacc.y lex.l
CPPFLAGS+=	-I. -I${.CURDIR} 			\
		-I${NETBSDSRCDIR}/lib/libc
#		-I${.CURDIR}/../../lib/libc/include \
#		-DNBCHAR_MAX=127 -DHOSTPROG
YHEADER=	#1

.ifndef HOSTPROG
.include <bsd.prog.mk>
COPTS.man.c+=	${GCC_NO_FORMAT_TRUNCATION}
.endif
