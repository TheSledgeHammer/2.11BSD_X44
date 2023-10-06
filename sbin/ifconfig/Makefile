#	$NetBSD: Makefile,v 1.14 2002/09/20 21:21:53 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifconfig
MAN=	ifconfig.8

INCS=	ifconfig.h	
SRCS=	af_inet.c af_ns.c ifconfig.c \
		ifieee80211.c ifmedia.c ifvlan.c \
		tunnel.c

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+=		af_inet6.c
.endif

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
