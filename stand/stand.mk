#	$211BSD: Makefile,v 1.0 2021/05/25 23:59:27 Exp $

.include <bsd.obj.mk>		# Pull in OBJDIR name rules.
.include <bsd.loader.mk>

LIBSRC?= 	${NETBSDSRCDIR}/sys

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

.include 	"${KERNDIR}/Makefile"

### find out what to use for libsa
SA_AS=		library
SADIR=		${LIBSRC}/lib/libsa

.include 	"${SADIR}/Makefile.libsa"

.include <bsd.lib.mk>

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