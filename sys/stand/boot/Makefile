# $FreeBSD: src/sys/boot/Makefile,v 1.18 2003/08/16 02:48:20 obrien Exp $

.include <bsd.loader.mk>

SUBDIR= 	common dloader

.if ${LOADER_MACHINE} == "i386" || ${LOADER_MACHINE_ARCH} == "i386"
SUBDIR+= 	i386

# Enable the following.
HAVE_BCACHE=	yes
HAVE_ISABUS=	yes
HAVE_PNP=		yes
.endif

INCSDIR= 	/usr/include/stand/boot

.include <bsd.kinc.mk>
		
