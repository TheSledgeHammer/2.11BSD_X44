# $FreeBSD$

BOOT_DLOADER?=			yes
LOADER_NET_SUPPORT?=	no
LOADER_CD9660_SUPPORT?=	no
LOADER_EXT2FS_SUPPORT?=	no
LOADER_MSDOS_SUPPORT?=	no
LOADER_UFS_SUPPORT?=	yes

.include <bsd.init.mk>

LOADER?=		loader
PROG=			${LOADER}.sym
NEWVERSWHAT?=	"bootstrap loader" i386
#VERSION_FILE=	${.CURDIR}/../loader/version

.PATH:			${BOOTSRC}/i386/loader

# architecture-specific loader code
SRCS=			main.c conf.c

# Include bcache code.
HAVE_BCACHE=	yes

# Enable PnP and ISA-PnP code.
HAVE_PNP=		yes
HAVE_ISABUS=	yes

.if exists(${.CURDIR}/help.i386)
HELP_FILES=		${.CURDIR}/help.i386
.endif


# Always add MI sources
.include		"${BOOTSRC}/loader.mk"

CLEANFILES+=	${LOADER} ${LOADER}.bin

ORG=			0x0

CFLAGS+=		-Wall
LDFLAGS+=		-static ${LDFLAGS_ORG} -Wl,--gc-sections

# i386 standalone support library
LIBI386=		${BOOTOBJ}/i386/libi386/libi386.a
CFLAGS+=		-I${BOOTSRC}/i386

# BTX components
LOADER_ADDRESS?=0x200000
LDFLAGS+=	    -nostdlib
LDFLAGS.lld+=	-Wl,--no-rosegment
MK_PIE:=	    no

BTXDIR=			${BOOTOBJ}/i386/btx
BTXLDR=			${BTXDIR}/btxldr/btxldr
BTXKERN=		${BTXDIR}/btx/btx
BTXCRT=			${BTXDIR}/lib/crt0.o

BTXSRC=			${BOOTSRC}/i386/btx
BTXLIB=			${BTXSRC}/lib

CFLAGS+=		-I${BTXLIB}

# compact binary with no padding between text, data, bss
LDSCRIPT=	    ${BOOTSRC}/i386/boot.ldscript
LDFLAGS_ORG=	-Wl,--defsym,ORG=${ORG},-T,${LDSCRIPT}
LDFLAGS_BIN=	-e start ${LDFLAGS_ORG} -Wl,-N,-S,--oformat,binary
.if ${LINKER_FEATURES:Mbuild-id} != ""
LDFLAGS_BIN+=	-Wl,--build-id=none
.endif
LD_FLAGS_BIN=	-static -N --gc-sections

#.include "../Makefile.inc"

# Debug me!
#CFLAGS+=		-g
#LDFLAGS+=		-g

${LOADER}: ${LOADER}.bin ${BTXLDR} ${BTXKERN}
	btxld -v -f aout -e ${LOADER_ADDRESS} -o ${.TARGET} -l ${BTXLDR} \
		-b ${BTXKERN} ${LOADER}.bin

${LOADER}.bin: ${LOADER}.sym
	${STRIPBIN} -R .comment -R .note -o ${.TARGET} ${.ALLSRC}
	
FILES+=	${LOADER}
FILESMODE_${LOADER}= ${BINMODE} -b

# XXX crt0.o needs to be first for pxeboot(8) to work
OBJS=	${BTXCRT}

.include <bsd.prog.mk>