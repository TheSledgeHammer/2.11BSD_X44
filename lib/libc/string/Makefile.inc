#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1996/3/20
#
# This is the Makefile for the 'string' functions.  New routines ported from 
# 4.4BSD's libc/string directory go here but existing libc/gen files are 
# being left where they are.
#
# string sources
.PATH: ${ARCHDIR}/string ${.CURDIR}/string

SRCS+=	bcmp.c bcopy.c bzero.c ffs.c index.c rindex.c setmode.c 		\
		strcasecmp.c strcat.c strcmp.c strcpy.c strcspn.c strdup.c		\
		strerror.c strlen.c strncat.c strncmp.c strncpy.c strpbrk.c 	\
		strsep.c strspn.c strstr.c strtok.c swab.c

.include "${ARCHDIR}/string/Makefile.inc"

MAN+= 	bcmp.3 bcopy.3 bzero.3 ffs.3 rindex.3 setmode.3 strcasecmp.3 	\
		strcat.3 strcmp.3 strcpy.3 strcspn.3 strdup.3 strlen.3			\
		strpbrk.3 stsep.3 strspn.3 strstr.3 strtok.3 swab.3

