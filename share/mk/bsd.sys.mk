#	$NetBSD: bsd.sys.mk,v 1.111.2.1.2.1 2005/05/31 21:41:02 tron Exp $
#
# Build definitions used for NetBSD source tree builds.

.if !defined(_BSD_SYS_MK_)
_BSD_SYS_MK_=1

.if !empty(.INCLUDEDFROMFILE:MMakefile*)
error1:
	@(echo "bsd.sys.mk should not be included from Makefiles" >& 2; exit 1)
.endif
.if !defined(_BSD_OWN_MK_)
error2:
	@(echo "bsd.own.mk must be included before bsd.sys.mk" >& 2; exit 1)
.endif

.if defined(WARNS)
.if ${WARNS} > 0
CFLAGS+=	-Wall -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
#CFLAGS+=	-Wmissing-declarations -Wredundant-decls -Wnested-externs
# Add -Wno-sign-compare.  -Wsign-compare is included in -Wall as of GCC 3.3,
# but our sources aren't up for it yet. Also, add -Wno-traditional because
# gcc includes #elif in the warnings, which is 'this code will not compile
# in a traditional environment' warning, as opposed to 'this code behaves
# differently in traditional and ansi environments' which is the warning
# we wanted, and now we don't get anymore.
CFLAGS+=	-Wno-sign-compare -Wno-traditional
# XXX Delete -Wuninitialized by default for now -- the compiler doesn't
# XXX always get it right.
CFLAGS+=	-Wno-uninitialized
.endif
.if ${WARNS} > 1
CFLAGS+=	-Wreturn-type -Wswitch -Wshadow
.endif
.if ${WARNS} > 2
CFLAGS+=	-Wcast-qual -Wwrite-strings
# Readd -Wno-sign-compare to override -Wextra with clang
CFLAGS+=	-Wno-sign-compare
.endif

.if ${WARNS} > 3 && (defined(HAVE_GCC) || defined(HAVE_LLVM))
.if ${WARNS} > 4
CFLAGS+=	-Wold-style-definition
.endif
.if ${WARNS} > 5
CFLAGS+=	-Wconversion
.endif
.endif

.if defined(WFORMAT) && defined(FORMAT_AUDIT)
.if ${WFORMAT} > 1
CFLAGS+=	-Wnetbsd-format-audit -Wno-format-extra-args
.endif
.endif

CPPFLAGS+=	${AUDIT:D-D__AUDIT__}
CFLAGS+=	${CWARNFLAGS} ${NOGCCERROR:D:U-Werror}
LINTFLAGS+=	${DESTDIR:D-d ${DESTDIR}/usr/include}

# Helpers for cross-compiling
HOST_CC?=	cc
HOST_CFLAGS?=	-O
HOST_COMPILE.c?=${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} -c
HOST_LINK.c?=	${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}

HOST_CPP?=	cpp
HOST_CPPFLAGS?=

HOST_LD?=	ld
HOST_LDFLAGS?=

STRIPPROG?=	strip


.SUFFIXES:	.m .o .ln .lo

# Objective C
# (Defined here rather than in <sys.mk> because `.m' is not just
#  used for Objective C source)
.m:
		${LINK.m} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.m.o:
		${COMPILE.m} ${.IMPSRC}

# Host-compiled C objects
.c.lo:
		${HOST_COMPILE.c} -o ${.TARGET} ${.IMPSRC}


.if defined(PARALLEL) || defined(LPREFIX)
LPREFIX?=yy
LFLAGS+=-P${LPREFIX}
# Lex
.l:
		${LEX.l} -o${.TARGET:R}.${LPREFIX}.c ${.IMPSRC}
		${LINK.c} -o ${.TARGET} ${.TARGET:R}.${LPREFIX}.c ${LDLIBS} -ll
		rm -f ${.TARGET:R}.${LPREFIX}.c
.l.c:
		${LEX.l} -o${.TARGET} ${.IMPSRC}
.l.o:
		${LEX.l} -o${.TARGET:R}.${LPREFIX}.c ${.IMPSRC}
		${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.${LPREFIX}.c 
		rm -f ${.TARGET:R}.${LPREFIX}.c
.l.lo:
		${LEX.l} -o${.TARGET:R}.${LPREFIX}.c ${.IMPSRC}
		${HOST_COMPILE.c} -o ${.TARGET} ${.TARGET:R}.${LPREFIX}.c 
		rm -f ${.TARGET:R}.${LPREFIX}.c
.endif

# Yacc
.if defined(YHEADER) || defined(YPREFIX)
.if defined(YPREFIX)
YFLAGS+=-p${YPREFIX}
.endif
.if defined(YHEADER)
YFLAGS+=-d
.endif
.y:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		${LINK.c} -o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
		rm -f ${.TARGET:R}.tab.c ${.TARGET:R}.tab.h
.y.h: 	${.TARGET:R}.c
.y.c:
		${YACC.y} -o ${.TARGET} ${.IMPSRC}
.y.o:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
		rm -f ${.TARGET:R}.tab.c ${TARGET:R}.tab.h
.y.lo:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		${HOST_COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
		rm -f ${.TARGET:R}.tab.c ${TARGET:R}.tab.h
.elif defined(PARALLEL)
.y:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		${LINK.c} -o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
		rm -f ${.TARGET:R}.tab.c
.y.c:
		${YACC.y} -o ${.TARGET} ${.IMPSRC}
.y.o:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
		rm -f ${.TARGET:R}.tab.c
.y.lo:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		${HOST_COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
		rm -f ${.TARGET:R}.tab.c
.endif

.endif	# !defined(_BSD_SYS_MK_)
