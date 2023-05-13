#	$NetBSD: Makefile,v 1.285 2020/08/20 21:30:46 riastradh Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	boot0cfg btxld chown chroot config link mtree postinstall pwd_mkdb unlink zic

.if ${TOOLCHAIN_MISSING} != "no"
SUBDIR+= mdsetimage
.endif

.include <bsd.subdir.mk>
