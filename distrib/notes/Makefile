#	$NetBSD: Makefile,v 1.46 2018/08/07 08:26:42 martin Exp $
#

.include <bsd.own.mk>

TARGETS+=	release

.ifndef ALLNOTES
.if exists(${RELEASEMACHINE}/Makefile)
SUBDIR=	${RELEASEMACHINE}
.endif
.else
SUBDIR+= amd64
SUBDIR+= i386
.endif

.include <bsd.subdir.mk>
