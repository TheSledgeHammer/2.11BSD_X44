#	@(#)Makefile	5.2 (Berkeley) 7/5/92

.include <bsd.own.mk>

PROG=	mount_ufml
SRCS=	mount_ufml.c

MOUNT=  ${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+= -I${NETBSDSRCDIR}/sys -I${MOUNT}
.PATH:  ${MOUNT}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
