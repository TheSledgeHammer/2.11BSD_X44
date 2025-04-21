#	$211BSD: Makefile,v 1.0 2025/03/27 18:51:42 Exp $

.include <bsd.own.mk>

LIB= netbsd
WARNS?=	5

CPPFLAGS+=	-I${.CURDIR}

INCS+=      core.h dkio.h filio.h statvfs.h
INCSDIR=	/usr/include/sys

SRCS+=      core_netbsd.c statvfs.c

.include <bsd.lib.mk>
