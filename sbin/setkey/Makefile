# $NetBSD: Makefile,v 1.3 2002/08/19 10:17:00 lukem Exp $

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	setkey
SRCS=	setkey.c parse.y token.l

#CFLAGS+=-g
LDADD+=	-ll -ly
DPADD+=	${LIBL} ${LIBY}
CLEANFILES+=	y.tab.c y.tab.h key_test.o keytest
YHEADER=	yes

#SCRIPTS=	scriptdump

all: ${PROG} scriptdump

CPPFLAGS+=-DIPSEC_DEBUG -DINET6 -DYY_NO_UNPUT
CPPFLAGS+=-I. -I${.CURDIR}
LDADD+= -lipsec
DPADD+=	${LIBIPSEC}
CLEANFILES+=	scriptdump

# libpfkey.
# ipsec_strerror.c is for avoiding shlib reference to non-exported function.
.PATH: ${NETBSDSRCDIR}/usr.lib/libipsec ${NETBSDSRCDIR}/sys/netipsec
SRCS+= pfkey.c pfkey_dump.c key_debug.c ipsec_strerror.c
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.lib/libipsec -I${NETBSDSRCDIR}/sys/netipsec

MAN=	setkey.8
LOCALPREFIX=	/usr/pkg

scriptdump: scriptdump.pl
	sed -e 's#@LOCALPREFIX@#${LOCALPREFIX}#' < $> > scriptdump

.include <bsd.prog.mk>
