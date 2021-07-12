# $FreeBSD$

NO_OBJ=t

.include <bsd.init.mk>

SUBDIR=		boot0 boot2 btx cdboot libi386 loader

.if ${LOADER_GPT_SUPPORT} == "yes"
SUBDIR+= gptboot
.endif

.if ${LOADER_MBR_SUPPORT} == "yes"
SUBDIR+= mbr
.endif		

INCSDIR= 	/usr/include/stand/boot/i386

.include <bsd.kinc.mk>