# $FreeBSD$

NO_OBJ=t

.include <bsd.init.mk>
.include <bsd.loader.mk>

.if ${LOADER_INTERP} == "dloader"
LOADER_INTERP=	dloader
.endif

SUBDIR=			boot0 boot2 btx cdboot libi386 loader

# Enable the following.
HAVE_BCACHE=	yes
HAVE_ISABUS=	yes
HAVE_PNP=		yes

.if ${LOADER_GPT_SUPPORT} == "yes"
SUBDIR+= gptboot
.endif

.if ${LOADER_MBR_SUPPORT} == "yes"
SUBDIR+= mbr
.endif		

INCSDIR= 	/usr/include/stand/boot/arch/i386

.include <bsd.kinc.mk>
