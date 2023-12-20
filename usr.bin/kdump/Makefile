#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>		# for MKDYNAMICROOT & NETBSDSRCDIR

PROG=	kdump
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ktrace -I${NETBSDSRCDIR}/sys
SRCS=	kdump.c ioctl.c subr.c
.PATH:	${NETBSDSRCDIR}/usr.bin/ktrace
CLEANFILES+=ioctl.c
WFORMAT=1

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

ioctl.c: mkioctls
	/bin/sh ${.CURDIR}/mkioctls > ioctl.c

.include <bsd.prog.mk>
