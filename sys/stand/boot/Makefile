# $FreeBSD: src/sys/boot/Makefile,v 1.18 2003/08/16 02:48:20 obrien Exp $

.include <bsd.loader.mk>

SUBDIR= 		common dloader

.if ${LOADER_MACHINE} == "i386" || ${LOADER_MACHINE_ARCH} == "i386"
SUBDIR+= 		i386

# Enable the following.
.if ${HAVE_BCACHE} == "no"
HAVE_BCACHE=	yes
.endif
.if ${HAVE_ISABUS} == "no"
HAVE_ISABUS=	yes
.endif
.if ${HAVE_PNP} == "no"
HAVE_PNP=		yes
.endif
.endif

INCSDIR= 		/usr/include/stand/boot

.include <bsd.kinc.mk>
		