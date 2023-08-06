#	$NetBSD: Makefile,v 1.21 2023/06/03 09:09:22 lukem Exp $
#	@(#)Makefile	8.3 (Berkeley) 1/25/94

USE_FORT?= yes	# network server

PROG=	mountd
SRCS=	mountd.c #ipsec.c get_net.c
MAN=	exports.5 mountd.8 netgroup.5

#INETDDIR= ${.CURDIR}/../inetd

#CPPFLAGS+= -I${INETDDIR} -DIPSEC
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

COPTS.mountd.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

#.PATH: ${INETDDIR}