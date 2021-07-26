#	$NetBSD: Makefile,v 1.18 2003/10/03 18:46:22 wiz Exp $

WARNS=	3

.include <bsd.own.mk>

USE_SHLIBDIR=	yes
ZLIBDIR=${NETBSDSRCDIR}/usr.lib/zlib
.PATH: ${ZLIBDIR}
LIB=		z

SRCS=	adler32.c compress.c crc32.c deflate.c gzio.c infblock.c \
		infcodes.c inffast.c inflate.c inftrees.c infutil.c trees.c uncompr.c \
		zutil.c gzio_compat.c
MAN=	zlib.3

CPPFLAGS+=	-I${.CURDIR}
CLEANFILES+=	minigzip

INCS=		zconf.h zlib.h
INCSDIR=	/usr/include

FILES+=				zlib.pc
FILESOWN_zlib.pc=	${BINOWN}
FILESGRP_zlib.pc=	${BINGRP}
FILESMODE_zlib.pc=	${NONBINMODE}
FILESDIR_zlib.pc=	/usr/lib/pkgconfig

COPTS.infback.c+=	${GCC_NO_IMPLICIT_FALLTHRU}
COPTS.inflate.c+=	${GCC_NO_IMPLICIT_FALLTHRU}

.include <bsd.lib.mk>

test: minigzip
	echo hello world | ./minigzip | ./minigzip -d

# Note: CFLAGS ommitted by intention!
# This is to verify that zlib.h works standalone.
minigzip: ${ZLIBDIR}/test/minigzip.c libz.a
	${CC} -o minigzip ${ZLIBDIR}/test/minigzip.c libz.a
