#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1996/3/20
#
# This is the Makefile for the 'string' functions.  New routines ported from 
# 4.4BSD's libc/string directory go here but existing libc/gen files are 
# being left where they are.
#
# string sources
.PATH: ${ARCHDIR}/string ${.CURDIR}/string

SRCS+=	strcspn.c strpbrk.c strerror.c strsep.c strspn.c strstr.c strtok.c

.include "${ARCHDIR}/string/Makefile.inc"

