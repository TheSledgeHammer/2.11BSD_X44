#	$NetBSD: Makefile,v 1.87.2.2 2012/03/17 18:28:33 bouyer Exp $

NOLIBCSANITIZER=# defined
NOSANITIZER=	# defined
WARNS?=4

LDELFSO_MACHINE_ARCH?= ${MACHINE_ARCH}

# This needs to be before bsd.init.mk
.if defined(BSD_MK_COMPAT_FILE)
.include <${BSD_MK_COMPAT_FILE}>
.endif

# We are not building this with PIE
PIE_CFLAGS=
PIE_LDFLAGS=
.include <bsd.init.mk>			# for OBJECT_FMT definition
.include <bsd.shlib.mk>			# for SHLINKINSTALLDIR definition

.if defined(LDELFSO_MACHINE_CPU) && !empty(LDELFSO_MACHINE_CPU) && \
    exists(${.CURDIR}/arch/${LDELFSO_MACHINE_CPU})
ARCHSUBDIR=	${LDELFSO_MACHINE_CPU}
.else
ARCHSUBDIR=	${MACHINE_CPU}
.endif
M=		${.CURDIR}/arch/${ARCHSUBDIR}

.if ((${MACHINE_ARCH} == "alpha") ||					\
     (${MACHINE_CPU} == "arm") ||					\
     (${MACHINE_ARCH} == "hppa") ||					\
     (${MACHINE_ARCH} == "i386") ||					\
     (${MACHINE_ARCH} == "m68k") ||					\
     (${MACHINE_CPU} == "mips") ||					\
     (${MACHINE_ARCH} == "powerpc") ||					\
     (${MACHINE_CPU} == "sh3") ||					\
     (${MACHINE_ARCH} == "sparc") ||					\
     (${MACHINE_ARCH} == "sparc64") ||					\
     (${MACHINE_ARCH} == "x86_64") ||					\
     (${MACHINE_ARCH} == "vax")) &&					\
    ${OBJECT_FMT} == "ELF" && ${MKPIC} != "no"

LDFLAGS+=	${${ACTIVE_CC} == "clang":? -Wl,-Bsymbolic : -symbolic} \
		    -shared -nostartfiles -nodefaultlibs
LDFLAGS+=	-Wl,-static
LDFLAGS+=	-Wl,--warn-shared-textrel
LDFLAGS+=	-Wl,--no-undefined
LDFLAGS+=	-Wl,-Map=${.TARGET}.map

COPTS+=		-fvisibility=hidden

# Adds SRCS, CPPFLAGS, LDFLAGS, etc.  Must go first so MD startup source
# is first.
.if exists($M/Makefile.inc)
.include "$M/Makefile.inc"
.endif

# Support compat ld.elf_so.
.if defined(MLIBDIR)
PROG=		ld.elf_so-${MLIBDIR}
CPPFLAGS+=	-DRTLD_ARCH_SUBDIR=\"${MLIBDIR}\"
.else
PROG=		ld.elf_so
.endif

CLIBOBJ!=	cd ${NETBSDSRCDIR}/lib/libc && ${PRINTOBJDIR}

SRCS+=	rtld.c
SRCS+=  reloc.c 
SRCS+=  symbol.c
SRCS+=  xmalloc.c
SRCS+=  xprintf.c
SRCS+=  debug.c
SRCS+=  map_object.c
SRCS+=  load.c
SRCS+=  search.c
SRCS+=  headers.c
SRCS+=  paths.c
SRCS+=  expand.c

#.if ${USE_FORT} == "yes"
#.PATH.c: ${NETBSDSRCDIR}/lib/libc/misc
#SRCS+=		stack_protector.c
#.endif

.PATH.c: ${NETBSDSRCDIR}/lib/libc/stdlib
SRCS+=		exit.c

BINDIR=		${SHLINKINSTALLDIR}

CPPFLAGS+=	-DLIBDIR=\"${LIBDIR}\" -D_PATH_RTLD=\"${BINDIR}/${PROG}\"
CPPFLAGS+=	-I${.CURDIR} -I. -D_KERNTYPES
CPPFLAGS+=	-DRTLD_LOADER
CPPFLAGS+=	-DGNU_RELRO
CPPFLAGS+=	-D_RTLD_SOURCE
CPPFLAGS+=	-DCOMBRELOC
#CPPFLAGS+=	-DDEBUG
#CPPFLAGS+=	-DRTLD_DEBUG
#CPPFLAGS+=	-DRTLD_DEBUG_RELOC
.if !empty(LDELFSO_MACHINE_ARCH:Mearm*)
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libexecinfo
.endif
#DBG=		-g
DBG=		-O3

.if ${MACHINE_CPU} != "m68k" && ${MACHINE_CPU} != "sh3" && ${MACHINE_ARCH} != "vax"
COPTS+=		-fomit-frame-pointer
.endif

.if ${LDELFSO_MACHINE_ARCH} == "i386" || ${LDELFSO_MACHINE_ARCH} == "x86_64"
COPTS+=		-mno-3dnow -mno-mmx -mno-sse -mno-sse2 -mno-sse3
.endif

.if ${SHLIBDIR} != ${LIBDIR}
CPPFLAGS+=	-DRTLD_DEFAULT_LIBRARY_PATH=\"${SHLIBDIR}:${LIBDIR}\"
.endif

# rtld.c and symbol.c use alloca, so disable SSP warnings.
COPTS.rtld.c+=	-Wno-stack-protector
COPTS.symbol.c+=-Wno-stack-protector

LDADD+=		-L${CLIBOBJ} -L${DESTDIR}${LIBDIR}
.if ${MKPICLIB} != "no"
LDADD+=		-lc_pic
.if ${MKPICINSTALL} != "no"
DPADD+=		${LIBC_PIC}
.endif
DPADD+=		${CLIBOBJ}/libc_pic.a
.else
LDADD+=		-lc
DPADD+=		${CLIBOBJ}/libc.a
.endif

STRIPFLAG=

.PATH: $M

${PROG}: ${OBJS} ${DPADD}
	${_MKMSG_LINK} ${PROG}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS} ${LDADD}
.if ${MKDEBUG} != "no"
	(  ${OBJCOPY} --only-keep-debug ${.TARGET} ${.TARGET}.debug \
	   && ${OBJCOPY} --strip-debug -p -R .gnu_debuglink \
	   --add-gnu-debuglink=${.TARGET}.debug ${.TARGET} \
	) || (rm -f ${.TARGET}.debug; false)

_PROGDEBUG.${PROG} := ${PROG}.debug
.endif


.if ${SHLINKINSTALLDIR} != "/usr/libexec"
SYMLINKS+=	${SHLINKINSTALLDIR}/${PROG} /usr/libexec/${PROG}
.if ${MKDEBUG} != "no"
SYMLINKS+=	${DEBUGDIR}${BINDIR}/${PROG}.debug \
		${DEBUGDIR}/usr${BINDIR}/${PROG}.debug
.endif
.endif

.include <bsd.prog.mk>
.else

MAN=		ld.elf_so.1

.include <bsd.man.mk>
.endif
