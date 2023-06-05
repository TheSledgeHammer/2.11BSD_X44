#	$211BSD: Makefile,v 1.00

.include <bsd.loader.mk>

ARCHSUBDIR= ${MACHINE_CPU}

SUBDIR=	${MACHINE}

.if ${MACHINE} != ${ARCHSUBDIR}
.if exists(${ARCHSUBDIR})
SUBDIR+= ${ARCHSUBDIR}
.endif
.endif

INCSDIR= 	/usr/include/stand/boot/arch

.include <bsd.kinc.mk>
