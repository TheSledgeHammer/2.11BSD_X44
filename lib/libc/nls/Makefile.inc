#	$NetBSD: Makefile.inc,v 1.8 1996/05/13 23:29:32 jtc Exp $

.PATH: ${.CURDIR}/nls

SRCS+=	catclose.c catgets.c catopen.c
MAN+=	catclose.3 catgets.3 catopen.3
