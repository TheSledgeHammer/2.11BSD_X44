# $FreeBSD$

.if !defined(__BOOT_DEFS_MK__)
__BOOT_DEFS_MK__=${MFILE}

# We need to define all the MK_ options before including src.opts.mk
# because it includes bsd.own.mk which needs the right MK_ values,
# espeically MK_CTF.

MK_CTF=			no
MK_SSP=			no
MK_PROFILE=		no
MAN=
.if !defined(PIC)
NO_PIC=
INTERNALLIB=
.endif

#.include <src.opts.mk>
#.include <bsd.linker.mk>

WARNS?=			1

SYSDIR=			${SRCTOP}/sys
STANDDIR=		${SYSDIR}/stand
LIBKERN=		${KERNLIB}
LIBSA=			${SALIB}
BOOTSRC=		${STANDDIR}/boot
#EFISRC=		${BOOTSRC}/efi
#EFIINC=		${EFISRC}/include
#EFIINCMD=		${EFIINC}/${MACHINE}
DLOADER=		${BOOTSRC}/dloader
LDRSRC=			${BOOTSRC}/common
#UBOOTSRC=		${BOOTSRC}/uboot
LIBCSRC=		${SRCTOP}/lib/libc

BOOTOBJ=		${OBJTOP}/stand

# BINDIR is where we install
BINDIR?=		/boot

# Standard options:
CFLAGS+=		-nostdinc
#CFLAGS+=		-I${BOOTOBJ}/libsa
CFLAGS+=		-I${SASRC} -D_STANDALONE
CFLAGS+=		-I${SYSDIR}
# Spike the floating point interfaces
CFLAGS+=		-Ddouble=jagged-little-pill -Dfloat=floaty-mcfloatface
.if ${MACHINE_ARCH} == "i386"
# Slim down the image. This saves about 15% in size with clang 6 on x86
# Our most constrained /boot/loader env is BIOS booting on x86, where
# our text + data + BTX have to fit into 640k below the ISA hole.
# Experience has shown that problems arise between ~520k to ~530k.
CFLAGS.clang+=	-Oz
CFLAGS.gcc+=	-Os
CFLAGS+=		-ffunction-sections -fdata-sections
.endif

# These should be confined to loader.mk, but can't because uboot/lib
# also uses it. It's part of loader, but isn't a loader so we can't
# just include loader.mk
.if ${LOADER_DISK_SUPPORT:Uyes} == "yes"
CFLAGS+= 		-DLOADER_DISK_SUPPORT
.endif

.if ${MACHINE_CPUARCH} == "i386"
CFLAGS+=		-march=i386
CFLAGS.gcc+=	-mpreferred-stack-boundary=2
.endif

# Make sure we use the machine link we're about to create
CFLAGS+=		-I.

all: ${PROG}

.endif # __BOOT_DEFS_MK__