#	$OpenBSD: Makefile,v 1.6 2003/11/20 23:23:09 avsm Exp $

CPPFLAGS+=-Wall -Wmissing-prototypes -Wshadow
# for pcap-int.h
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.lib/libpcap
.PATH:	${NETBSDSRCDIR}/usr.lib/libpcap

CPPFLAGS+=-DHAVE_SNPRINTF=1 -DHAVE_STRLCPY=1 -DHAVE_VSNPRINTF=1

LDADD+= -lpcap -lutil
DPAPP+=	${LIBPCAP} ${LIBUTIL}

PROG=	pflogd
SRCS=	pflogd.c privsep.c privsep_fdpass.c
MAN=	pflogd.8

.include <bsd.prog.mk>
