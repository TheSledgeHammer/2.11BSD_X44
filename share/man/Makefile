#	$NetBSD: Makefile,v 1.34 2014/12/13 14:16:48 christos Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR+=	man0 man1 man2 man3 man3f man4 man5 man6 man7 man8 #man9
#SUBDIR+=	cat1 cat2 cat3 cat3f cat4 cat5 cat6 cat7 cat8 #cat9

WHATISDBDIR?=	/usr/share/man

.if ${MKMAKEMANDB} == "no"
makedb:
	${TOOL_MAKEWHATIS} ${DESTDIR}${WHATISDBDIR}
.if ${MKUNPRIVED} != "no"
	echo ".${WHATISDBDIR}/whatis.db type=file mode=0444 uname=${BINOWN} gname=${BINGRP}" \
	    | ${METALOG.add}
.endif
.else
makedb:
.endif

.else
makedb:
.endif

.include <bsd.subdir.mk>
