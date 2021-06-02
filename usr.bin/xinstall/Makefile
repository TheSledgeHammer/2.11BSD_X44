#	$OpenBSD: Makefile,v 1.9 2019/02/24 12:57:14 kn Exp $

.include <bsd.own.mk>

PROG=	xinstall
MAN=	install.1

.PATH:		${NETBSDSRCDIR}/usr.sbin/mtree
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.sbin/mtree

.if (${HOSTPROG:U} == "")
CPPFLAGS+=	-DHAVE_POSIX_SPAWN
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

COPTS.xinstall.c += -Wno-format-nonliteral

PROGNAME=install

CWARNFLAGS.gcc+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
