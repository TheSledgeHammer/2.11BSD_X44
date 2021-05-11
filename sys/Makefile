#	$NetBSD: Makefile,v 1.86 2020/05/18 21:19:35 jmcneill Exp $

.include <bsd.own.mk>

SUBDIR=	arch compat dev fs miscfs 	\
		net netimp netinet netns 	\
		sys ufs vm

.if make(obj) || make(cleandir) || ${MKBOOT} != "no"
SUBDIR+= stand/boot
.endif

.include <bsd.kinc.mk>
