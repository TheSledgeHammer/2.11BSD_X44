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
COMMONSRC=					${BOOTSRC}/common
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
BINDIR?=					/usr/mdec
#BINDIR?=					/boot

# Machine Dependent
.if ${MACHINE} == "i386" || ${MACHINE} == "x86_64"
HAVE_BCACHE?=				yes
HAVE_ISABUS?=				yes
HAVE_PNP?=					yes
.else
HAVE_BCACHE?=				no
HAVE_ISABUS?=				no
HAVE_PNP?=					no
.endif
LOADER_EFI_SUPPORT?=		no
LOADER_FDT_SUPPORT?=		no

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
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${BOOTARCH}/${MACHINE}/lib${MACHINE} -I${LIBSRC}/lib/libsa
CPPFLAGS+= -D_STANDALONE
LDFLAGS+=  -nostdlib

.if ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=-m32
LDFLAGS+=-Wl,-m,elf_i386
LIBKERN_ARCH=i386
KERNMISCMAKEFLAGS="LIBKERN_ARCH=i386"
.endif

### find out what to use for libkern
KERN_AS=	library
.include 	"${LIBSRC}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS+="SA_USE_LOADFILE=yes"
.include 	"${LIBSRC}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

### find out what to use for libz
Z_AS=		library
.include    "${LIBSRC}/lib/libz/Makefile.inc"
LIBZ=		${ZLIB}

.if !empty(HELP_FILES)
HELP_FILES+=	${COMMONSRC}/help.common

CLEANFILES+=	loader.help
FILES+=			loader.help

loader.help: ${HELP_FILES}
		${TOOL_CAT} ${HELP_FILES} | ${TOOL_AWK} -f ${COMMONSRC}/merge_help.awk
.endif

.endif	# !defined(_BSD_LOADER_MK_)
