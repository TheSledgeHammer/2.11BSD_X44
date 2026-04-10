#	$NetBSD: Makefile,v 1.21 2003/10/26 07:25:35 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	route
MAN=	route.8
SRCS=	route.c show.c keywords.c

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

# Appletalk Networks
#CPPFLAGS+=-DAPPLETALK

# ISO/OSI Networks
CPPFLAGS+=-DISO
NETISOSRC= ${NETBSDSRCDIR}/lib/libc/inet/iso
.PATH:  ${NETISOSRC}
CPPFLAGS+=-I${NETISOSRC}

# Xerox NS Networks
CPPFLAGS+=-DXNS

# X.25 Networks
CPPFLAGS+=-DCCITT

COPTS.route.c+= ${GCC_NO_FORMAT_TRUNCATION}

# The Makefile over in ../../distrib/utils/x_route
# would like keywords.[ch] to always exist here, so
# they are now checked in as sources.
#
# CPPFLAGS+=-I.
# CLEANFILES+= keywords.c keywords.h
# keywords.c keywords.h : keywords.sh
# 	${HOST_SH} keywords.sh

.include <bsd.prog.mk>
