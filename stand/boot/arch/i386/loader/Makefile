# $FreeBSD$

.include <bsd.loader.mk>

LOADER?=		loader
PROG=			${LOADER}.sym
INTERNALPROG=
NEWVERSWHAT?=	"bootstrap loader" i386
#VERSION_FILE=	${.CURDIR}/../loader/version
#
# There's 640k - 40k maximum space, less however much memory the BIOS uses.  A
# non-random survey suggests that 20k-25k is a good value for 'most' machines.
# We also need to subtract maximum stack usage (20-25k).
#
# So 640k - 40k - 25k - 25k = 550k = 563,200 bytes, but use 550,000 below for
# some extra buffer for more complex setups and/or wider BIOS lomem variation.
#
# Some systems use more stack or have BIOS reserve more RAM (or both), and will
# need to set this lower in /etc/src.conf if the above buffer is too small.
#
# sudo dd if=/dev/mem bs=1 iseek=0x413 count=2 | hd -d
#
# will tell you how many kiB of lomem are available.
#
LOADERSIZE?=	550000		# Largest known safe size for loader.bin

.PATH:			${BOOTARCH}/i386/loader

# architecture-specific loader code
SRCS=			main.c conf.c

.if exists(${.CURDIR}/help.i386)
HELP_FILES=		${.CURDIR}/help.i386
.endif

# Always add MI sources

CLEANFILES+=	${LOADER} ${LOADER}.bin

ORG=			0x0

CFLAGS+=		-Wall
LDFLAGS+=		-static ${LDFLAGS_ORG} -Wl,--gc-sections

# i386 standalone support library
LIBI386=		${BOOTARCH}/i386/libi386/libi386.a
CFLAGS+=		-I${BOOTARCH}/i386

# Debug me!
#CFLAGS+=		-g
#LDFLAGS+=		-g

${LOADER}: ${LOADER}.bin ${BTXLDR} ${BTXKERN}
	btxld -v -f aout -e ${LOADER_ADDRESS} -o ${.TARGET} -l ${BTXLDR} \
		-b ${BTXKERN} ${LOADER}.bin

${LOADER}.bin: ${LOADER}.sym
	${STRIPBIN} -R .comment -R .note -o ${.TARGET} ${.ALLSRC}
	@set -- `ls -l ${.TARGET}` ; x=$$((${LOADERSIZE}-$$5)); \
	    echo "$$x bytes available"; test $$x -ge 0

#.if ${LOADER_INTERP} == ${LOADER_DEFAULT_INTERP}
#LINKS+=		${BINDIR}/${LOADER} ${BINDIR}/loader
#.endif	
FILES+=	${LOADER}
FILESMODE_${LOADER}= ${BINMODE} -b

# XXX crt0.o needs to be first for pxeboot(8) to work
LDFLAGS+=	${BTXCRT}

DPADD=	${LIBI386} ${LIBSA}
LDADD=	${LIBI386} ${LIBSA}

.include <bsd.prog.mk>
.include <bsd.kinc.mk>
