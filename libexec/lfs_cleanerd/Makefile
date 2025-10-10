#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	lfs_cleanerd
#-I${NETBSDSRCDIR}/sys/ufs/lfs
CPPFLAGS+= -I${.CURDIR} -DDIAGNOSTIC
MAN=	lfs_cleanerd.8
SRCS=	cleanerd.c lfs_cksum.c library.c misc.c print.c

#DPADD+=  ${LIBUTIL}
#LDADD+=  -lutil

.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs

.include <bsd.prog.mk>
