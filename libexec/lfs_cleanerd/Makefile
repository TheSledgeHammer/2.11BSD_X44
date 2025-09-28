#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	lfs_cleanerd
MAN8=	lfs_cleanerd.8
SRCS=	cleanerd.c lfs_cksum.c library.c misc.c print.c

.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs

DPADD+=  ${LIBUTIL}
LDADD+=  -lutil

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/ufs/lfs -I${.CURDIR} -DDIAGNOSTIC

.include <bsd.prog.mk>
