# $FreeBSD: src/sys/boot/Makefile,v 1.18 2003/08/16 02:48:20 obrien Exp $

#EFISRC=		${BOOTSRC}/efi
#EFIINC=		${EFISRC}/include
#EFIINCMD=		${EFIINC}/${MACHINE}
DLOADER=		${BOOTSRC}/dloader
LDRSRC=			${BOOTSRC}/common
#UBOOTSRC=		${BOOTSRC}/uboot
LIBCSRC=		${SRCTOP}/lib/libc

BOOTOBJ=		${STANDDIR}

# BINDIR is where we install
BINDIR?=		/boot

# Standard options:
# Options used when building standalone components
CFLAGS+=		-nostdinc
CFLAGS+=		-ffreestanding -fshort-wchar -Wformat -D_STANDALONE
LDFLAGS+=		#-nostdlib
CPPFLAGS+=		-D_STANDALONE

### find out what to use for libkern
KERN_AS=		library
.include 		"${LIBKERN}/Makefile.inc"

### find out what to use for libsa
SA_AS=			library
SAMISCMAKEFLAGS+="SA_USE_LOADFILE=yes"
.include 		"${LIBSA}/Makefile.inc"

.include <bsd.subdir.mk>
		