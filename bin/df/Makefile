#	@(#)Makefile	8.3 (Berkeley) 5/8/95

#.include <bsd.own.mk>

PROG=	df
SRCS=	df.c vfslist.c
BINGRP=	operator
BINMODE=2555

MOUNT=	${.CURDIR}/../../sbin/mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
