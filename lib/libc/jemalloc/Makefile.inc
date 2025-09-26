#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1995/1/12
#

# jemalloc sources
.PATH: ${ARCHDIR}/jemalloc ${.CURDIR}/jemalloc

.if (${USE_JEMALLOC} != "no")
SRCS+= jemalloc.c
.endif

MAN+= jemalloc.3
