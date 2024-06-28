#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	quotacheck
SRCS=	quotacheck.c preen.c
MAN=	quotacheck.8

FFSSRC=  ${NETBSDSRCDIR}/sys/ufs/ffs
FSCKSRC= ${NETBSDSRCDIR}/sbin/fsck

.PATH:  ${FFSSRC} ${FSCKSRC}

.include <bsd.prog.mk>
