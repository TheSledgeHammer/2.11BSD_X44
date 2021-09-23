#	@(#)Makefile	8.1 (Berkeley) 6/18/93

.include <bsd.own.mk>

PROG=	newlfs
CFLAGS+=-I/sys/ufs/lfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN=	newlfs.8
.PATH:	/sys/ufs/lfs ${.CURDIR}/../disklabel

.include <bsd.prog.mk>
