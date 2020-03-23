#	$Id: Makefile,v 1.2 1994/08/02 07:38:17 davidg Exp $

.include <bsd.own.mk>

# The boot loader
.if	exists(${.CURDIR}/boot)
SUBDIR=	boot
.endif

SUBDIR= arch net netimp netinet netns ufs vfs vm

.include <bsd.subdir.mk>