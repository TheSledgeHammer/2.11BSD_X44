#	$NetBSD: Makefile,v 1.2 2019/07/24 14:07:17 joerg Exp $

SUBDIR=		lib .WAIT bin

CONFIG_DIR:=	${.PARSEDIR}/dist

.include <bsd.own.mk>

.if (${MKXZ} != "no")
CONFIG_ARGS+= --without-lzma
.endif
CONFIG_ARGS+= --without-xml2
CONFIG_ARGS+= --host=${MACHINE_GNU_ARCH}--netbsd

run-configure:
	[ ! -d tmp ] || rm -r tmp
	mkdir -p tmp
	cd tmp && CONFIG_SITE= ${CONFIG_DIR}/configure ${CONFIG_ARGS} \
	    CC=${CC:Q} CFLAGS=${CFLAGS:N-Werror:Q} CPPFLAGS=${CPPFLAGS:Q} LDFLAGS=${LDFLAGS:Q}
	rm -r tmp

.include <bsd.own.mk>
.include <bsd.subdir.mk>
