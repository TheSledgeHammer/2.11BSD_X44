#	$211BSD: Makefile,v 1.0 2021/05/25 23:59:27 Exp $
#  	FreeBSD BTX Bootloader 

.if !defined(_BSD_LOADER_MK_)
_BSD_LOADER_MK_=1

.include <bsd.obj.mk>
.include <bsd.own.mk>

# Directories
LIBSRC?= 					${NETBSDSRCDIR}/sys

BOOTSTAND?=					stand
BOOTSRC?=					stand/boot
BOOTARCH?=					${BOOTSRC}/arch

EFISRC=						${BOOTSRC}/efi
EFIINC=						${EFISRC}/include
EFIINCMD=					${EFIINC}/${MACHINE}
FDTSRC=						${BOOTSRC}/fdt
LDRSRC=						${BOOTSRC}/common
LIBLUASRC=					${BOOTSRC}/liblua
LUASRC=						/contrib/lua/dist
DLOADER=					${BOOTSRC}/dloader
LUA=						${BOOTSRC}/lua
LIBCSRC=					/lib/libc

BOOTOBJ=					${BOOTSTAND}

# Interpreter Files (loader, menu & conf)
# Accommodates for different naming conventions (i.e. dloader)

INTERP_CONF=				loader.conf
INTERP_MENU=				dloader.menu
INTERP_RC=					dloader.rc

# Interpreter Support
MK_LOADER_LUA=				no

#
# Have a sensible default
#
.if ${MK_LOADER_LUA} == "yes"
LOADER_DEFAULT_INTERP?=lua
.else
LOADER_DEFAULT_INTERP?=dloader
.endif
LOADER_INTERP?=	${LOADER_DEFAULT_INTERP}

# BINDIR is where we install
BINDIR?=					/boot

# Machine support
LOADER_MACHINE?=			${MACHINE}
LOADER_MACHINE_ARCH?=		${MACHINE_ARCH}

# Machine Dependent
MK_EFI=						no
MK_FDT=						no

# Filesystem support
LOADER_CD9660_SUPPORT?=		yes
LOADER_MSDOS_SUPPORT?=		yes
LOADER_UFS_SUPPORT?=		yes
LOADER_EXT2FS_SUPPORT?=  	no

# Partition support
LOADER_GPT_SUPPORT?=		yes
LOADER_MBR_SUPPORT?=		yes

# Network support
LOADER_NET_SUPPORT?=		no

# Standard options:
# Options used when building standalone components
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S}
CPPFLAGS+= -D_STANDALONE

COPTS+=	-ffreestanding
COPTS+=	-fno-stack-protector
COPTS+=	-fno-unwind-tables
CWARNFLAGS+= -Werror
CWARNFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

### find out what to use for libkern
KERN_AS=	library
KERNDIR=	${LIBSRC}/lib/libkern

#.include 	"${KERNDIR}/Makefile"

### find out what to use for libsa
SA_AS=		library
SADIR=		${LIBSRC}/lib/libsa

#.include 	"${SADIR}/Makefile.libsa"

CLEANFILES+=	vers.c
VERSION_FILE?=	${.CURDIR}/version

vers.c: ${LDRSRC}/newvers.sh ${VERSION_FILE}
	sh ${LDRSRC}/newvers.sh ${REPRO_FLAG} ${VERSION_FILE} \
	    ${NEWVERSWHAT}

.if !empty(HELP_FILES)
HELP_FILES+=	${LDRSRC}/help.common

CLEANFILES+=	loader.help
FILES+=			loader.help

loader.help: ${HELP_FILES}
	cat ${HELP_FILES} | awk -f ${LDRSRC}/merge_help.awk > ${.TARGET}
.endif

.include <bsd.sys.mk>

.endif	# !defined(_BSD_LOADER_MK_)
