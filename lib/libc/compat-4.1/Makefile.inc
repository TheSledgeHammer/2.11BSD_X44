#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4.1 (2.11BSD GTE) 1/21/95

# compat-4.1 sources
.PATH: 	${ARCHDIR}/compat-4.1 ${.CURDIR}/compat-4.1
SRCS+=	ftime.c gtty.c nice.c pause.c rand.c stty.c times.c utime.c

MAN+=   nice.3 pause.3 rand.3 times.3 utime.3
