#	$NetBSD: bsd.prog.mk,v 1.198.2.1 2004/05/22 17:36:11 he Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.ifndef HOSTPROG

.include <bsd.init.mk>
.include <bsd.shlib.mk>
.include <bsd.gcc.mk>
.include <bsd.sanitizer.mk>

.if defined(PROG_CXX)
PROG=	${PROG_CXX}
.endif

##### Sanitizer specific flags.

CFLAGS+=	${SANITIZERFLAGS} ${LIBCSANITIZERFLAGS}
CXXFLAGS+=	${SANITIZERFLAGS} ${LIBCSANITIZERFLAGS}
LDFLAGS+=	${SANITIZERFLAGS}

##### Basic targets
realinstall:	proginstall scriptsinstall
clean:			cleanprog

##### PROG specific flags.
COPTS+=     ${COPTS.${PROG}}
CPPFLAGS+=  ${CPPFLAGS.${PROG}}
CXXFLAGS+=  ${CXXFLAGS.${PROG}}
LDADD+=     ${LDADD.${PROG}}
LDFLAGS+=   ${LDFLAGS.${PROG}}
LDSTATIC+=  ${LDSTATIC.${PROG}}

##### Default values
CPPFLAGS+=	${DESTDIR:D-nostdinc ${CPPFLAG_ISYSTEM} ${DESTDIR}/usr/include}
CXXFLAGS+=	${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEMXX} ${DESTDIR}/usr/include/g++}
CFLAGS+=	${COPTS}
MKDEP_SUFFIXES?=	.o .ln

# ELF platforms depend on crti.o, crtbegin.o, crtend.o, and crtn.o
.if ${OBJECT_FMT} == "ELF"
.ifndef LIBCRTBEGIN
LIBCRTBEGIN=	${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crti.o ${_GCC_CRTBEGIN}
.MADE: ${LIBCRTBEGIN}
.endif
.ifndef LIBCRTEND
LIBCRTEND=	${_GCC_CRTEND} ${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crtn.o
.MADE: ${LIBCRTEND}
.endif
_SHLINKER=	${SHLINKDIR}/ld.so
.endif

.ifndef LIBCRT0
LIBCRT0=	${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crt0.o
.MADE: ${LIBCRT0}
.endif

.ifndef LIBCRTI
LIBCRTI=	${DESTDIR}/usr/lib/${MLIBDIR:D${MLIBDIR}/}crti.o
.MADE: ${LIBCRTI}
.endif

##### Installed system library definitions
#     E.g. LIBC?=${DESTDIR}/usr/lib/libc.a
#     etc..
#

USRSLASHLIB=	${DESTDIR}/usr/lib # Modern BSD's (i.e. 4.4 and above)
USRDOTLIB=		${DESTDIR}/usr.lib # Older BSD's (i.e. 4.3 and below)

_USRSLASHLIBLIST= \
			C \
			C_PIC \
			COM_ERR \
			COMPAT \
			EXPAT \
			FETCH \
			GCC \
			GNUMALLOC \
			L \
			MAGIC \
            		STDC++ \
            		SUPC++
			
.for _var in ${_USRSLASHLIBLIST}
.ifndef LIB${_var}
LIB${_var}:=	 ${USRSLASHLIB}/lib${_var:tl}.a
.MADE: ${LIB${_var}}
.endif
.endfor

_USRDOTLIBLIST=	\
			BZ2 \
			CRYPT \
			CRYPTO \
			CURSES \
			DBM \
			EDIT \
			EXECINFO \
			FORM \
			FORTRAN \
			IPSEC \
			KVM \
			M \
			MENU \
			MP \
			OM \
			PANEL \
			PCAP \
			PCI \
			PTHREAD \
			RESOLV \
			SKEY \
			SS \
			STUBS \
			TERMCAP \
			UTIL \
			VMF \
			Y \
			Z

.for _var in ${_USRDOTLIBLIST}
.ifndef LIB${_var}
LIB${_var}:=	 ${USRDOTLIB}/lib${_var:tl}.a
.MADE: ${LIB${_var}}
.endif
.endfor

.ifndef LIBTERMINFO
LIBTERMINFO = ${USRDOTLIB}/libtermcap.a
.MADE: 		${LIBTERMINFO}
.endif

.ifndef LIBSTDCXX
LIBSTDCXX=	${USRSLASHLIB}/libstdc++.a
.MADE: 		${LIBSTDCXX}
.endif

##### Build and install rules
.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.o:
	${CC} -E ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${CC} ${CFLAGS} -c x.c -o ${.TARGET}
	@rm -f x.c

.cc.o .cpp.o .cxx.o .C.o:
	${CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.cc
	@${CXX} ${CXXFLAGS} -c x.cc -o ${.TARGET}
	@rm -f x.cc
.endif

.if !empty(_APPEND_SRCS:M[Yy][Ee][Ss])
SRCS+=		${SRCS.${_P}}	# For bsd.dep.mk
.endif

.if defined(PROG)
.if defined(PROG_CXX)
SRCS?=		${PROG}.cc
.else
SRCS?=		${PROG}.c
.endif

DPSRCS+=		${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
DPSRCS+=		${YHEADER:D${SRCS:M*.y:.y=.h}}
CLEANFILES+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${YHEADER:D${SRCS:M*.y:.y=.h}}

.if !empty(SRCS:N*.h:N*.sh:N*.fth)
OBJS+=		${SRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
LOBJS+=		${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
.endif

.if defined(OBJS) && !empty(OBJS)
.NOPATH: ${OBJS} ${PROG} ${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}

_PROGLDOPTS=
.if ${SHLINKDIR} != "/usr/libexec"	# XXX: change or remove if ld.so moves
.if ${OBJECT_FMT} == "ELF"
_PROGLDOPTS+=	-Wl,-dynamic-linker=${_SHLINKER}
.endif
.endif
.if ${SHLIBDIR} != "/usr/lib"
_PROGLDOPTS+=	-Wl,-rpath-link,${DESTDIR}${SHLIBDIR}:${DESTDIR}/usr/lib \
		-R${SHLIBDIR} \
		-L${DESTDIR}${SHLIBDIR}
.elif ${SHLIBINSTALLDIR} != "/usr/lib"
_PROGLDOPTS+=	-Wl,-rpath-link,${DESTDIR}${SHLIBINSTALLDIR}:${DESTDIR}/usr/lib \
		-L${DESTDIR}${SHLIBINSTALLDIR}
.endif

.if defined(PROG_CXX)
_CCLINK=	${CXX}
.if ${USE_LIBSTDCXX} == "no"
_SUPCXX=	-lsupc++ -lm
.else
_SUPCXX=	-lstdc++ -lm
.endif
.else
_CCLINK=	${CC}
.endif

.gdbinit:
	rm -f .gdbinit
.if defined(DESTDIR) && !empty(DESTDIR)
	echo "set solib-absolute-prefix ${DESTDIR}" > .gdbinit
.else
	touch .gdbinit
.endif
.for __gdbinit in ${GDBINIT}
	echo "source ${__gdbinit}" >> .gdbinit
.endfor

${OBJS} ${LOBJS}: ${DPSRCS}

${PROG}: .gdbinit ${LIBCRT0} ${OBJS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${DPADD}
.if !commands(${PROG})
	${_MKTARGET_LINK}
.if defined(DESTDIR)
	${_CCLINK} -Wl,-nostdlib \
	    ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${_PROGLDOPTS} \
	    -B${_GCC_CRTDIR}/ -B${DESTDIR}/usr/lib/  \
	    ${OBJS} ${LDADD} \
	    -L${_GCC_LIBGCCDIR} -L${DESTDIR}/usr/lib
.else
	${_CCLINK} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${_PROGLDOPTS} ${OBJS} ${LDADD}
.endif	# defined(DESTDIR)
.endif	# !commands(${PROG})

${PROG}.ro: ${OBJS} ${DPADD}
	${LD} -r -dc -o ${.TARGET} ${OBJS}

.endif	# defined(OBJS) && !empty(OBJS)

.if !defined(MAN)
MAN=	${PROG}.1
.endif	# !defined(MAN)
.endif	# defined(PROG)

realall: ${PROG} ${SCRIPTS}

cleanprog: .PHONY cleanobjs cleanextra
	rm -f a.out [Ee]rrs mklog core *.core .gdbinit ${PROG}

cleanobjs: .PHONY
.if defined(OBJS) && !empty(OBJS)
	rm -f ${OBJS} ${LOBJS}
.endif

cleanextra: .PHONY
.if defined(CLEANFILES) && !empty(CLEANFILES)
	rm -f ${CLEANFILES}
.endif

.if defined(PROG) && !target(proginstall)
PROGNAME?=${PROG}

proginstall:: ${DESTDIR}${BINDIR}/${PROGNAME}
.PRECIOUS: ${DESTDIR}${BINDIR}/${PROGNAME}

__proginstall: .USE
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${STRIPFLAG} ${SYSPKGTAG} ${.ALLSRC} ${.TARGET}

.if ${MKUPDATE} == "no"
${DESTDIR}${BINDIR}/${PROGNAME}! ${PROG} __proginstall
.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${BINDIR}/${PROGNAME}! .MADE
.endif
.else
${DESTDIR}${BINDIR}/${PROGNAME}: ${PROG} __proginstall
.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${BINDIR}/${PROGNAME}: .MADE
.endif
.endif
.endif

.if !target(proginstall)
proginstall::
.endif
.PHONY:		proginstall

.if defined(SCRIPTS) && !target(scriptsinstall)
SCRIPTSDIR?=${BINDIR}
SCRIPTSOWN?=${BINOWN}
SCRIPTSGRP?=${BINGRP}
SCRIPTSMODE?=${BINMODE}

scriptsinstall:: ${SCRIPTS:@S@${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}@}
.PRECIOUS: ${SCRIPTS:@S@${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}@}

__scriptinstall: .USE
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} \
	    -o ${SCRIPTSOWN_${.ALLSRC:T}:U${SCRIPTSOWN}} \
	    -g ${SCRIPTSGRP_${.ALLSRC:T}:U${SCRIPTSGRP}} \
	    -m ${SCRIPTSMODE_${.ALLSRC:T}:U${SCRIPTSMODE}} \
	    ${SYSPKGTAG} ${.ALLSRC} ${.TARGET}

.for S in ${SCRIPTS:O:u}
.if ${MKUPDATE} == "no"
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}! ${S} __scriptinstall
.if !defined(BUILD) && !make(all) && !make(${S})
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}! .MADE
.endif
.else
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}: ${S} __scriptinstall
.if !defined(BUILD) && !make(all) && !make(${S})
${DESTDIR}${SCRIPTSDIR_${S}:U${SCRIPTSDIR}}/${SCRIPTSNAME_${S}:U${SCRIPTSNAME:U${S:T:R}}}: .MADE
.endif
.endif
.endfor
.endif

.if !target(scriptsinstall)
scriptsinstall::
.endif
.PHONY:		scriptsinstall

lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:C/-L[  ]*/-L/Wg:M-L*} ${LOBJS} ${LDADD}
.endif

##### Pull in related .mk logic
LINKSOWN?= ${BINOWN}
LINKSGRP?= ${BINGRP}
LINKSMODE?= ${BINMODE}
.include <bsd.man.mk>
.include <bsd.nls.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.links.mk>
.include <bsd.sys.mk>
.include <bsd.dep.mk>
.include <bsd.clang-analyze.mk>
.include <bsd.clean.mk>

${TARGETS}:	# ensure existence

.endif	# HOSTPROG
