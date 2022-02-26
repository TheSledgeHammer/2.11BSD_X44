#	$211BSD: Makefile,v 1.00

.include <bsd.own.mk>

ARCHSUBDIR= ${MACHINE_CPU}

SUBDIR=	${LOADER_MACHINE}

.if (${LOADER_MACHINE} || ${LOADER_MACHINE_ARCH}) != ${ARCHSUBDIR}
.if exists(${ARCHSUBDIR})
SUBDIR+= ${ARCHSUBDIR}
.endif
.endif

INCSDIR= 	/usr/include/stand/boot/arch

.include <bsd.kinc.mk>