#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?= 3

.include <bsd.own.mk>

PROG=	dumplfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
MAN=	dumplfs.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys/ufs/lfs

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs

.include <bsd.prog.mk>
