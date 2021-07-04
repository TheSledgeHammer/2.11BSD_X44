# $FreeBSD$

.PATH: ${.CURDIR}/../../../${MACHINE_ARCH}/${MACHINE_ARCH}

LIB=	efi
INTERNALLIB=

SRCS=	autoload.c bootinfo.c copy.c delay.c devicename.c efi_console.c efifs.c \
		efinet.c libefi.c time.c

CFLAGS+= -I${.CURDIR}/../include
CFLAGS+= -I${.CURDIR}/../include/${MACHINE_ARCH}
CFLAGS+= -I${.CURDIR}/../../../../lib/libstand

# Pick up the bootstrap header for some interface items
CFLAGS+= -I${.CURDIR}/../../common

# Make the disk code more talkative
.if defined(BOOT_DISK_DEBUG)
CFLAGS+= -DDISK_DEBUG
.endif

.include <bsd.lib.mk>
