#	$OpenBSD: Makefile,v 1.9 2019/02/24 12:57:14 kn Exp $

.include <bsd.own.mk>

PROG=	xinstall
SRCS=	${HOSTPROG:D:Ustat_flags.c} xinstall.c getid.c
MAN=	install.1


.PATH:		${NETBSDSRCDIR}/bin/ls ${NETBSDSRCDIR}/usr.sbin/mtree
CPPFLAGS+=	-I${NETBSDSRCDIR}/bin/ls -I${NETBSDSRCDIR}/usr.sbin/mtree

.if (${HOSTPROG:U} == "")
CPPFLAGS+=	-DHAVE_POSIX_SPAWN
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

COPTS.xinstall.c += -Wno-format-nonliteral

PROGNAME=install

CWARNFLAGS.gcc+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
