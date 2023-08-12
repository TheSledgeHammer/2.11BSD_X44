#	$NetBSD: Makefile,v 1.7 2012/06/05 13:41:23 tsutsui Exp $
# NetBSD Default: 	/usr/mdec
# FreeBSD Default: 	/boot

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-fdisk
HOST_SRCDIR=	sbin/fdisk
HOST_SRCS=		disklabel.c

HOST_CPPFLAGS=	-DDEFAULT_BOOTDIR=\"${DESTDIR}/boot\"

#.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
