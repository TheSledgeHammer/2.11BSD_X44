#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	mount_lfs
SRCS=	mount_lfs.c getmntopts.c
MAN=	mount_lfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
