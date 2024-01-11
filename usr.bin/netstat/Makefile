#	$NetBSD: Makefile,v 1.45.6.1 2019/08/19 15:56:49 martin Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/12/93

.include <bsd.own.mk>

USE_FORT?= yes	# setgid

PROG=	netstat
SRCS=	fast_ipsec.c if.c inet.c inet6.c ipsec.c \
		main.c mbuf.c mroute.c mroute6.c ns.c pfkey.c route.c unix.c rtutil.c
BINGRP=	kmem
BINMODE=2555
LDADD=	-lutil -lkvm
DPADD=	${LIBUTIL} ${LIBKVM}
CPPFLAGS+= -DIPSEC -I${.CURDIR}
CPPFLAGS+= -I${NETBSDSRCDIR}/sbin/pfctl
CPPFLAGS+= -I${NETBSDSRCDIR}/sbin/pflogd
CPPFLAGS+= -I${NETBSDSRCDIR}/sbin/route

.PATH:  ${.CURDIR}/../../sbin/route

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

#.include "../../compat/exec.mk"

.include <bsd.prog.mk>
