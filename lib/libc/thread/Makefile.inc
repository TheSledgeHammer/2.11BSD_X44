#	$NetBSD: Makefile.inc,v 1.4 2013/04/05 20:15:42 christos Exp $

.include <bsd.own.mk>

# Our sources
.PATH: ${.CURDIR}/thread

SRCS+=	__isthreaded.c thread_atfork.c thread-libc.c thread-stub.c
