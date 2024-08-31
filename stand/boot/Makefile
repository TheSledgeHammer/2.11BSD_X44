# $FreeBSD: src/sys/boot/Makefile,v 1.18 2003/08/16 02:48:20 obrien Exp $

.include <bsd.loader.mk>

SUBDIR= 	common dloader

.if ${MK_LOADER_LUA} == "yes"
SUBDIR+=	lua
.endif
.if ${LOADER_EFI_SUPPORT} == "yes"
SUBDIR+=	efi
.endif
.if ${LOADER_FDT_SUPPORT} == "yes"
SUBDIR+=	fdt
.endif

SUBDIR+=	arch

.include <bsd.inc.mk>
		
