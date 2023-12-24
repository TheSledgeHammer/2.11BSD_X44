# $FreeBSD$

.include <bsd.init.mk>
.include "${.CURDIR}/../../Makefile.inc" 

PROG=	btxldr
INTERNALPROG=
SRCS=	btxldr.S

CFLAGS+=-DLOADER_ADDRESS=${LOADER_ADDRESS}
CFLAGS+=-I${.CURDIR}/../../common

.if defined(BTXLDR_VERBOSE)
CFLAGS+=-DBTXLDR_VERBOSE
.endif

ORG=${LOADER_ADDRESS}
LDFLAGS+=${LDFLAGS_BIN}

.include <bsd.prog.mk>
