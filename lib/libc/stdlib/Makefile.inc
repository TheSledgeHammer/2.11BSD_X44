#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1995/1/12
#
# This is the Makefile for 'stdlib'.  New routines ported from 4.4BSD's
# libc/stdlib directory go here but existing libc/gen files are being left
# where they are.
#

# stdlib sources
.PATH: ${ARCHDIR}/stdlib ${.CURDIR}/stdlib

SRCS+=	abort.c abs.c atoi.c atol.c calloc.c getenv.c getopt.c getsubopt.c 	\
		heapsort.c malloc.c qsort.c random.c setenv.c strtol.c strtoul.c 	\
		system.c

.include "${ARCHDIR}/stdlib/Makefile.inc"

MAN+= 	abort.3 abs.3 atof.3 atoi.3 atol.3 calloc.3 getopt.3 getsubopt.3 	\
		malloc.3 qsort.3 random.3 strol.3 stroul.3 system.3

