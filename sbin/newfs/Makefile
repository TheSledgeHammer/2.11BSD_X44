#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	newfs
SRCS=	dkcksum.c getmntopts.c newfs.c mkfs.c
MAN=	newfs.8

MOUNT=	${NETBSDSRCDIR}/sbin/mount
CFLAGS+=-DMFS -I${MOUNT}
.PATH:	${MOUNT} ${NETBSDSRCDIR}/sbin/disklabel

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mount_mfs.8 newfs.8 mfs.8

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
