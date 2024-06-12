#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	systat

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat ${NETBSDSRCDIR}/usr.bin/who

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat -DSUPPORT_UTMP -DSUPPORT_UTMPX \
		-I${NETBSDSRCDIR}/usr.bin/who -D_KMEMUSER
	
SRCS=	cmds.c cmdtab.c disks.c fetch.c iostat.c keyboard.c vmstat.c main.c \
		mbufs.c netcmds.c netstat.c pigs.c swap.c
DPADD=	${LIBCURSES} ${LIBTERMINFO} ${LIBM} ${LIBKVM}
LDADD=	-lcurses -lterminfo -lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
