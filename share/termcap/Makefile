#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2.4 (2.11BSD GTE) 1997/9/22
#
# reorder gives an editor command for most common terminals
# (in reverse order from n'th to 1'st most commonly used)
# to move them to the front of termcap
#

MAN=	termcap.5

CLEANFILES= termcap.db
realall: termcap.db
FILES=termcap.db termcap.src map3270
FILESNAME_termcap.src=termcap
FILESDIR= ${BINDIR}/misc

.include <bsd.endian.mk>
.if ${TARGET_ENDIANNESS} == "1234"
CAP_MKDB_ENDIAN=	-l
.elif ${TARGET_ENDIANNESS} == "4321"
CAP_MKDB_ENDIAN=	-b
.else
CAP_MKDB_ENDIAN=
.endif

termcap.db: termcap.src
	${_MKTARGET_CREATE}
	${TOOL_CAP_MKDB} ${CAP_MKDB_ENDIAN} -f termcap ${.ALLSRC}

.include <bsd.prog.mk>
