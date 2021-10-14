#
# Copyright (c) 1985 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.2.1 (2.11BSD) 1996/10/21
#
#
#  Makefile for -man macros
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93
#	$NetBSD: Makefile,v 1.32 2019/12/24 23:41:05 uwe Exp $

# IF YOU ADD TO MSRCS, MAKE SURE YOU UPDATE `CATDEPS' IN bsd.man.mk!
MSRCS=	doc2html
MMACS=  ${MSRCS:S@$@.tmac@g}
INCS=	${MMACS}
INCSDIR=${BINDIR}/tmac
CLEANFILES+= ${MMACS}

TMACDIR?=   ${.CURDIR}
STRIP_SED = ${TMACDIR}/../me/strip.sed

SRCS=	tmac.a tmac.an tmac.an.new tmac.an6n tmac.an6t tmac.ayday \
	    tmac.cp tmac.imagen tmac.os tmac.r tmac.s tmac.scover \
	    tmac.sdisp tmac.skeep tmac.srefs tmac.vcat

all:	${SRCS}

install:${SRCS}
	-mkdir -p ${DESTDIR}/usr/share/tmac
	-chmod a+r,a+x ${DESTDIR}/usr/share/tmac
	install -c -m 444 ${SRCS} ${DESTDIR}/usr/share/tmac

clean:

realall: ${MMACS}

.for i in ${MSRCS}
$i.tmac: $i
	${_MKTARGET_CREATE}
	${TOOL_SED} -f ${STRIP_SED} < ${.ALLSRC} > ${.TARGET}
.endfor

.include <bsd.prog.mk>
