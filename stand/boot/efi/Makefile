# $FreeBSD$

.include <bsd.loader.mk>

SUBDIR=		boot1 include libefi

CFLAGS+=	-ffreestanding -fshort-wchar -Wformat
LDFLAGS+=	-nostdlib

.include <bsd.kinc.mk>
