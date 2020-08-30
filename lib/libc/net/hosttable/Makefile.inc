
#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4 (Berkeley) 9/5/85
#
# machine-independent net sources
.PATH: 	${.CURDIR}/net/hosttable
SRCS+=	gethnamadr.c gethostent.c
