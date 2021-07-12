#	$NetBSD: Makefile.inc,v 1.9 2001/03/31 19:00:56 kleink Exp $

.PATH: ${.CURDIR}/time

SRCS+=	ctime.c strftime.c
MAN+=		ctime.3 strftime.3
CFLAGS+=-DALL_STATE -DUSG_COMPAT

MLINKS+= ctime.3
