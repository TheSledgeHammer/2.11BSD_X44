# $FreeBSD$

CFLAGS+= -I${LDRSRC}

SRCS+=	boot.c commands.c console.c devopen.c environment.c
SRCS+=	fileload.c getopts.c interp_backslash.c interp_parse.c interp.c
SRCS+=	ls.c misc.c pager.c panic.c readdir.c
SRCS+=  smbios.c strdup.c strspn.c strtol.c ufsread.c

.if ${MACHINE} == "i386"
SRCS+= 	load_exec.c load_ksyms.c
.endif

.if ${LOADER_NET_SUPPORT:Uno} == "no"
SRCS+= 	dev_net.c
.endif

.if defined(HAVE_BCACHE)
SRCS+=  bcache.c
.endif

# Machine-independant ISA PnP
.if defined(HAVE_ISABUS)
SRCS+=	isapnp.c
.endif
.if defined(HAVE_PNP)
SRCS+=	pnp.c
.endif

.if ${BOOT_DLOADER} == "yes"
.include 		"dloader/Makefile.inc"
.endif

# Filesystem support
.if ${LOADER_CD9660_SUPPORT:Uno} == "yes"
CFLAGS+=		-DLOADER_CD9660_SUPPORT
.endif
.if ${LOADER_EXT2FS_SUPPORT:Uno} == "no"
CFLAGS+=		-DLOADER_EXT2FS_SUPPORT
.endif
.if ${LOADER_MSDOS_SUPPORT:Uno} == "yes"
CFLAGS+=		-DLOADER_MSDOS_SUPPORT
.endif
.if ${LOADER_UFS_SUPPORT:Uyes} == "yes"
CFLAGS+=		-DLOADER_UFS_SUPPORT
.endif

# Network related things
.if ${LOADER_NET_SUPPORT:Uno} == "yes"
CFLAGS+=		-DLOADER_NET_SUPPORT
.endif

# Partition support
.if ${LOADER_GPT_SUPPORT:Uyes} == "no"
CFLAGS+= 		-DLOADER_GPT_SUPPORT
.endif
.if ${LOADER_MBR_SUPPORT:Uyes} == "yes"
CFLAGS+= 		-DLOADER_MBR_SUPPORT
.endif

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
