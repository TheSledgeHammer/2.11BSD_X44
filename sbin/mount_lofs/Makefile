#	@(#)Makefile	5.2 (Berkeley) 7/5/92

.include <bsd.own.mk>

PROG=	mount_lofs
SRCS=	mount_lofs.c
MAN=	mount_lofs.8

MOUNT=  ${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:  ${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
