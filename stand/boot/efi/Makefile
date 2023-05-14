# $FreeBSD$

#.include <bsd.loader.mk>
.include <stand.mk>

SUBDIR=		boot1 include libefi

INCSDIR= 	/usr/include/stand/boot/efi

CFLAGS+=	-ffreestanding -fshort-wchar -Wformat
LDFLAGS+=	-nostdlib

.include <bsd.kinc.mk>
