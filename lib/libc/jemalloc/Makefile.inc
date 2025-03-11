#	$NetBSD: Makefile.inc,v 1.82.2.3 2016/12/08 07:43:44 snj Exp $
#	from: @(#)Makefile.inc	8.3 (Berkeley) 2/4/95

# jemalloc sources
.PATH: ${ARCHDIR}/jemalloc ${.CURDIR}/jemalloc

SRCS+= memalign.c jemalloc.c

#MAN+= jemalloc.3 posix_memalign.3
