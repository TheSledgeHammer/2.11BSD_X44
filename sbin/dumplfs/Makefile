#	@(#)Makefile	8.1 (Berkeley) 6/18/93
.include <bsd.own.mk>

PROG=	dumplfs
CFLAGS+=-I/sys/ufs/lfs
SRCS=	dumplfs.c lfs_cksum.c misc.c
.PATH:	/sys/ufs/lfs
MAN=	dumplfs.8

.include <bsd.prog.mk>
