#	$NetBSD: sys.mk,v 1.33.2.2 1997/11/05 05:37:41 thorpej Exp $
#	@(#)sys.mk	8.2 (Berkeley) 3/21/94

unix?=			We run 2.11BSD.

.SUFFIXES: 		.out .a .o .c .cc .C .cxx .cpp .F .f .r .y .l .s .S .cl .p .h .sh .m4

.LIBS:			.a

AR?=			ar
ARFLAGS?=		rl
RANLIB?=		ranlib
MV?=			mv -f

AS?=			as
AFLAGS?=
COMPILE.s?=		${CC} ${AFLAGS} -c
LINK.s?=		${CC} ${AFLAGS} ${LDFLAGS}
COMPILE.S?=		${CC} ${AFLAGS} ${CPPFLAGS} -c -traditional-cpp
LINK.S?=		${CC} ${AFLAGS} ${CPPFLAGS} ${LDFLAGS}

CC?=			cc
.if ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE_ARCH} == "arm" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_ARCH} == "armeb" || \
    ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "m68k" || \
    ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb" || \
    ${MACHINE_ARCH} == "ns32k" || \
    ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "sh5el" || ${MACHINE_ARCH} == "sh5eb" || \
    ${MACHINE_ARCH} == "sparc" || \
    ${MACHINE_ARCH} == "sparc64" || \
    ${MACHINE_ARCH} == "vax"
DBG?=			-O2
.else
DBG?=			-O
.endif
CFLAGS?=		${DBG}
COMPILE.c?=		${CC} ${CFLAGS} ${CPPFLAGS} -c
LINK.c?=		${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS}

# Strip flags unsupported by C++ compilers
# Remove -Wsystem-headers because C++ headers aren't clean of warnings
CXXSTRIPFLAGS?= N-Wno-traditional:N-Wstrict-prototypes:N-Wmissing-prototypes:N-Wno-pointer-sign:N-ffreestanding:N-std=gnu[0-9][0-9]:N-Wold-style-definition:N-Wno-format-zero-length:N-Wsystem-headers
CXX?=			c++
CXXFLAGS?=		${CFLAGS:${CXXSTRIPFLAGS}}
COMPILE.cc?=	${CXX} ${CXXFLAGS} ${CPPFLAGS} -c
LINK.cc?=		${CXX} ${CXXFLAGS} ${CPPFLAGS} ${LDFLAGS}

OBJC?=			${CC}
OBJCFLAGS?=		${CFLAGS}
COMPILE.m?=		${OBJC} ${OBJCFLAGS} ${CPPFLAGS} -c
LINK.m?=		${OBJC} ${OBJCFLAGS} ${CPPFLAGS} ${LDFLAGS}

CPP?=			cpp
CPPFLAGS?=	

FC?=			f77
FFLAGS?=		-O
RFLAGS?=
COMPILE.f?=		${FC} ${FFLAGS} -c
LINK.f?=		${FC} ${FFLAGS} ${LDFLAGS}
COMPILE.F?=		${FC} ${FFLAGS} ${CPPFLAGS} -c
LINK.F?=		${FC} ${FFLAGS} ${CPPFLAGS} ${LDFLAGS}
COMPILE.r?=		${FC} ${FFLAGS} ${RFLAGS} -c
LINK.r?=		${FC} ${FFLAGS} ${RFLAGS} ${LDFLAGS}

INSTALL?=		install

LD?=			ld
LDFLAGS?=

LEX?=			lex
LFLAGS?=
LEX.l?=			${LEX} ${LFLAGS}

LINT?=			lint
LINTFLAGS?=		-chapbrxzgFS

LORDER?=		lorder

MAKE?=			make

NM?=			nm

PC?=			pc
PFLAGS?=
COMPILE.p?=		${PC} ${PFLAGS} ${CPPFLAGS} -c
LINK.p?=		${PC} ${PFLAGS} ${CPPFLAGS} ${LDFLAGS}

SHELL?=			sh

SIZE?=			size

TSORT?= 		tsort -q

YACC?=			yacc
YFLAGS?=
YACC.y?=		${YACC} ${YFLAGS}

# C
.c:
		${LINK.c} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.c.o:
		${COMPILE.c} ${.IMPSRC}
.c.a:
		${COMPILE.c} ${.IMPSRC}
		${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
		rm -f ${.PREFIX}.o
.c.ln:
		${LINT} ${LINTFLAGS} \
	    ${CPPFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    -i ${.IMPSRC}

# C++
.cc .cpp .cxx .C:
		${LINK.cc} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.cc.o .cpp.o .cxx.o .C.o:
		${COMPILE.cc} ${.IMPSRC}
.cc.a .cpp.a .cxx.a .C.a:
		${COMPILE.cc} ${.IMPSRC}
		${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
		rm -f ${.PREFIX}.o

# Fortran/Ratfor
.f:
		${LINK.f} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.f.o:
		${COMPILE.f} ${.IMPSRC}
.f.a:
		${COMPILE.f} ${.IMPSRC}
		${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
		rm -f ${.PREFIX}.o

.F:
		${LINK.F} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.F.o:
		${COMPILE.F} ${.IMPSRC}
.F.a:
		${COMPILE.F} ${.IMPSRC}
		${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
		rm -f ${.PREFIX}.o

.r:
		${LINK.r} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.r.o:
		${COMPILE.r} ${.IMPSRC}
.r.a:
		${COMPILE.r} ${.IMPSRC}
		${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
		rm -f ${.PREFIX}.o

# Pascal
.p:
		${LINK.p} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.p.o:
		${COMPILE.p} ${.IMPSRC}
.p.a:
		${COMPILE.p} ${.IMPSRC}
		${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
		rm -f ${.PREFIX}.o

# Assembly
.s:
		${LINK.s} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.s.o:
		${COMPILE.s} ${.IMPSRC}
.s.a:
		${COMPILE.s} ${.IMPSRC}
		${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
		rm -f ${.PREFIX}.o
.S:
		${LINK.S} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.S.o:
		${COMPILE.S} ${.IMPSRC}
.S.a:
		${COMPILE.S} ${.IMPSRC}
		${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
		rm -f ${.PREFIX}.o

# Lex
.l:
		${LEX.l} ${.IMPSRC}
		${LINK.c} -o ${.TARGET} lex.yy.c ${LDLIBS} -ll
		rm -f lex.yy.c
.l.c:
		${LEX.l} ${.IMPSRC}
		mv lex.yy.c ${.TARGET}
.l.o:
		${LEX.l} ${.IMPSRC}
		${COMPILE.c} -o ${.TARGET} lex.yy.c
		rm -f lex.yy.c

# Yacc
.y:
		${YACC.y} ${.IMPSRC}
		${LINK.c} -o ${.TARGET} y.tab.c ${LDLIBS}
		rm -f y.tab.c
.y.c:
		${YACC.y} ${.IMPSRC}
		mv y.tab.c ${.TARGET}
.y.o:
		${YACC.y} ${.IMPSRC}
		${COMPILE.c} -o ${.TARGET} y.tab.c
		rm -f y.tab.c

# Shell
.sh:
		rm -f ${.TARGET}
		cp ${.IMPSRC} ${.TARGET}
		chmod a+x ${.TARGET}
