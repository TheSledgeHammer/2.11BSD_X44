# $FreeBSD: src/sys/boot/Makefile,v 1.18 2003/08/16 02:48:20 obrien Exp $

.include <bsd.loader.mk>

SUBDIR= 	arch common

.if ${MK_LOADER_LUA} == "yes"
SUBDIR+=	lua
.endif

.if ${MK_LOADER_DLOADER} == "yes"
SUBDIR+=	dloader
.endif

.if ${MK_EFI} == "yes"
SUBDIR+=	efi fdt
.endif

INCSDIR= 	/usr/include/stand/boot

.include <bsd.kinc.mk>
		
