#	$NetBSD: Makefile,v 1.14 2002/09/20 21:21:53 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	ifconfig
MAN=	ifconfig.8

INCS=	ifconfig.h	
SRCS=	af_inet.c af_ns.c ifcarp.c ifconfig.c \
	ifieee80211.c ifmedia.c iftunnel.c ifvlan.c

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+=		af_inet6.c
.endif

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

DPADD+=${LIBUTIL}
LDADD+=-lutil

COPTS.af_inet.c+= ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.af_inet6.c+= ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.af_ns.c+= ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.ifconfig.c+= ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.ifieee80211.c+= ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.ifmedia.c+= ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.iftunnel.c+= ${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
