# $FreeBSD$

NO_OBJ=t

.include <bsd.init.mk>
.include <bsd.loader.mk>

SUBDIR=	btx libi386
SUBDIR=	.WAIT

.if ${LOADER_MBR_SUPPORT} == "yes"
SUBDIR+= mbr pmbr
.endif

SUBDIR+= boot0 boot0sio boot2 cdboot isoboot

.if ${LOADER_GPT_SUPPORT} == "yes"
SUBDIR+= gptboot
.endif	

SUBDIR+= loader	

INCSDIR= 	/usr/include/stand/boot/arch/i386

.include <bsd.kinc.mk>
