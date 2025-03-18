#	$NetBSD: Makefile.inc,v 1.82.2.3 2016/12/08 07:43:44 snj Exp $
#	from: @(#)Makefile.inc	8.3 (Berkeley) 2/4/95

# jemalloc sources
.PATH: ${ARCHDIR}/jemalloc ${.CURDIR}/jemalloc

.if (${USE_JEMALLOC} != "no")
SRCS+= jemalloc.c
.endif

MAN+= jemalloc.3
