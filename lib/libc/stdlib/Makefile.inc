#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1995/1/12
#
# This is the Makefile for 'stdlib'.  New routines ported from 4.4BSD's
# libc/stdlib directory go here but existing libc/gen files are being left
# where they are.
#

# stdlib sources
.PATH: ${ARCHDIR}/stdlib ${.CURDIR}/stdlib

SRCS+=	getopt.c getsubopt.c strtol.c strtoul.c

.include "${ARCHDIR}/stdlib/Makefile.inc"
