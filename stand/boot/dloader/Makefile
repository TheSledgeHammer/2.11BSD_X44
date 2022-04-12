# $FreeBSD$

.include <bsd.loader.mk>

INTERP_MENU=	dloader.menu
INTERP_RC=		dloader.rc

INCSDIR= 	/usr/include/stand/boot/dloader

SRCS+=		subs.c cmds.c logo_beastie.c logo_fred.c

INCS=		dloader.h

# Allow machine specific loader.rc to be installed.
FILES= 		${INTERP_MENU} ${INTERP_CONF}
.if exists(${BOOTARCH}/${MACHINE}/loader/${INTERP_RC})
FILES+= 	${INTERP_RC}
.endif

.include <bsd.kinc.mk>