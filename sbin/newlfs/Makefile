#	@(#)Makefile	8.1 (Berkeley) 6/18/93

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	newlfs
SRCS=	dkcksum.c lfs.c lfs_cksum.c misc.c newfs.c
MAN=	newlfs.8

DISKLABEL= ${NETBSDSRCDIR}/sbin/disklabel

.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${DISKLABEL}

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.include <bsd.prog.mk>
