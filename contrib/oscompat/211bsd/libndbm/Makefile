#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3 (Berkeley) 4/30/86
#
.include <bsd.own.mk>

LIB=		ndbm
WARNS?=		5

CPPFLAGS+=	-I${.CURDIR}

SRCS= 		dbm.c ndbm.c
INCS= 		dbm.h ndbm.h
INCSDIR=	/usr/include

.include <bsd.lib.mk>