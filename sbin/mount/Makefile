#	$NetBSD: Makefile,v 1.28 2003/03/22 12:44:04 jdolecek Exp $
#	@(#)Makefile	8.6 (Berkeley) 5/8/95

.include <bsd.own.mk>

PROG=	mount
SRCS=	mount.c vfslist.c
SRCS+=  mount_ufs.c getmntopts.c
MAN=	mount.8

.if defined(RESCUEDIR)
CPPFLAGS+= -D_PATH_RESCUE=\"${RESCUEDIR}\"
.endif

LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <bsd.prog.mk>
