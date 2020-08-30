#	$NetBSD: bsd.sys.mk,v 1.12.2.2 1998/11/09 08:07:42 cgd Exp $
#
# Overrides used for NetBSD source tree builds.

.if !defined(_BSD_SYS_MK_)
_BSD_SYS_MK_=1

.if defined(WARNS)
.if ${WARNS} > 0
CFLAGS+=		-Wall -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith

CFLAGS+=		-Wno-sign-compare -Wno-traditional
# XXX Delete -Wuninitialized by default for now -- the compiler doesn't
# XXX always get it right.
CFLAGS+=		-Wno-uninitialized
.endif
.if ${WARNS} > 1
CFLAGS+=		-Wreturn-type -Wswitch -Wshadow
.endif
.if ${WARNS} > 2
CFLAGS+=		-Wcast-qual -Wwrite-strings
.endif
.endif

.if defined(WFORMAT) && defined(FORMAT_AUDIT)
.if ${WFORMAT} > 1
CFLAGS+=		-Wnetbsd-format-audit -Wno-format-extra-args
.endif
.endif

CPPFLAGS+=		${AUDIT:D-D__AUDIT__}
CFLAGS+=		${CWARNFLAGS} ${NOGCCERROR:D:U-Werror}
LINTFLAGS+=		${DESTDIR:D-d ${DESTDIR}/usr/include}

CFLAGS+=		${CPUFLAGS}

# Helpers for cross-compiling
HOST_CC?=		cc
HOST_CFLAGS?=	-O
HOST_COMPILE.c?=${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} -c
HOST_LINK.c?=	${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}

HOST_CXX?=		c++
HOST_CXXFLAGS?=	-O

HOST_CPP?=		cpp
HOST_CPPFLAGS?=

HOST_LD?=		ld
HOST_LDFLAGS?=

HOST_SH?=		sh

ELF2ECOFF?=		elf2ecoff
MKDEP?=			mkdep
OBJCOPY?=		objcopy
STRIPPROG?=		strip

AWK?=			awk

.SUFFIXES:		.o .ln .lo .c ${YHEADER:D.h}

# C
.c.o:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
.c.ln:
	${_MKTARGET_COMPILE}
	${LINT} ${LINTFLAGS} \
	    ${CPPFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS.${.IMPSRC:T}:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    -i ${.IMPSRC}

# C++
.cc.o .cpp.o .cxx.o .C.o:
	${_MKTARGET_COMPILE}
	${COMPILE.cc} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

# Objective C
# (Defined here rather than in <sys.mk> because `.m' is not just
#  used for Objective C source)
.m.o:
	${_MKTARGET_COMPILE}
	${COMPILE.m} ${.IMPSRC}

# Host-compiled C objects
# The intermediate step is necessary for Sun CC, which objects to calling
# object files anything but *.o
.c.lo:
	${_MKTARGET_COMPILE}
	${HOST_COMPILE.c} -o ${.TARGET}.o ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
	mv ${.TARGET}.o ${.TARGET}

# Assembly
.s.o:
	${_MKTARGET_COMPILE}
	${COMPILE.s} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.S.o:
	${_MKTARGET_COMPILE}
	${COMPILE.S} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

# Lex
LPREFIX?=	yy
LFLAGS+=	-P${LPREFIX}

.l.c:
	${_MKTARGET_LEX}
	${LEX.l} -o${.TARGET} ${.IMPSRC}

# Yacc
YFLAGS+=	${YPREFIX:D-p${YPREFIX}} ${YHEADER:D-d}

.y.c:
	${_MKTARGET_YACC}
	${YACC.y} -o ${.TARGET} ${.IMPSRC}

.ifdef YHEADER
.y.h: ${.TARGET:.h=.c}
.endif

.endif	# !defined(_BSD_SYS_MK_)