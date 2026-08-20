#	$NetBSD: Makefile,v 1.17 2021/12/25 13:54:13 mlelstv Exp $

PROG=	wsconsctl
SRCS=	display.c keyboard.c keysym.c map_parse.y map_scan.l \
		mouse.c util.c wsconsctl.c
MAN=	wsconsctl.8

YHEADER=	1
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/sys
DPSRCS+=	keysym.h
CLEANFILES+=	keysym.h

.include <bsd.prog.mk>

# Environment for scripts executed during build.
SCRIPT_ENV= \
	AWK=${TOOL_AWK:Q} \
	SED=${TOOL_SED:Q}

WSKEYSYM_H= usr/include/dev/misc/wscons/wsksymdef.h

keysym.h: mkkeysym.sh ${DESTDIR}/${WSKEYSYM_H}
	${_MKTARGET_CREATE}
	${SCRIPT_ENV} ${HOST_SH} ${.CURDIR}/mkkeysym.sh \
	  ${DESTDIR}/${WSKEYSYM_H} > keysym.h
