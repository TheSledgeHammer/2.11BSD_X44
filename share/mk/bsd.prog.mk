#	$NetBSD: bsd.prog.mk,v 1.198.2.1 2004/05/22 17:36:11 he Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.ifndef HOSTPROG

.include <bsd.init.mk>
.include <bsd.shlib.mk>
.include <bsd.gcc.mk>
.include <bsd.sanitizer.mk>

##### Sanitizer specific flags.

CFLAGS+=	${SANITIZERFLAGS} ${LIBCSANITIZERFLAGS}
CXXFLAGS+=	${SANITIZERFLAGS} ${LIBCSANITIZERFLAGS}
LDFLAGS+=	${SANITIZERFLAGS}

##### Basic targets
realinstall:	proginstall scriptsinstall
#clean:			cleanprog

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

CLEANFILES+= a.out [Ee]rrs mklog core *.core .gdbinit

.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.o:
	${CC} -E ${CPPFLAGS} ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${CC} ${CPPFLAGS} ${CFLAGS} -c x.c ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
	@rm -f x.c

.cc.o .cpp.o .cxx.o .C.o:
	${CXX} -E ${CPPFLAGS} ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@${MV} x.c x.cc
	@${CXX} ${CPPFLAGS} ${CXXFLAGS} -c x.cc ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
	@rm -f x.cc
.endif

.if defined(MKPIE) && (${MKPIE} != "no") && !defined(NOPIE)
CFLAGS+=	${PIE_CFLAGS}
AFLAGS+=	${PIE_AFLAGS}
LDFLAGS+=	${"${LDSTATIC.${.TARGET}}" == "-static" :? : ${PIE_LDFLAGS}}
.endif

CFLAGS+=	${COPTS}
.if ${MKDEBUG:Uno} != "no" && !defined(NODEBUG)
CFLAGS+=	-g
.endif
OBJCFLAGS+=	${OBJCOPTS}
MKDEP_SUFFIXES?=	.o .ln .d

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
_SHLINKER=	${SHLINKDIR}/ld.elf_so
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

_LIBLIST=   \
            archive \
            bz2 \
            c c_pic compat crypto curses cxx \
            dbm des \
            edit execinfo event event_openssl event_pthreads expat \
            fetch fl form fortran \
            g2c gcc gnuctf gnumalloc \
            intl ipsec \
            l lua \
            kvm \
            m magic menu mp \
            netbsd ns \
            objc om \
            panel pcap pci plan9 pthread \
            resolv rpcsvs \
            skey sqlite3 ssh ssl ssp stdc++ stubs supc++ \
            terminfo \
            util \
            vmf \
            wrap \
            y \
            z

.for _var in ${_LIBLIST}
.ifndef LIB${_var:tu}
LIB${_var:tu}:=	 ${DESTDIR}/usr/lib/lib${_var:S/xx/++/}.a
.MADE: ${LIB${_var:tu}}
.endif
.endfor

#.ifndef LIBSTDCXX
#LIBSTDCXX=	${DESTDIR}/usr/lib/libstdc++.a
#.MADE: 		${LIBSTDCXX}
#.endif

.if defined(RESCUEDIR)
CPPFLAGS+=	-DRESCUEDIR=\"${RESCUEDIR}\"
.endif

_PROGLDOPTS=
.if ${SHLINKDIR} != "/usr/libexec"	# XXX: change or remove if ld.so moves
.if ${OBJECT_FMT} == "ELF"
_PROGLDOPTS+=	-Wl,-dynamic-linker=${_SHLINKER}
.endif
.endif
.if ${SHLIBDIR} != "/usr/lib"
_PROGLDOPTS+=	-Wl,-rpath,${SHLIBDIR} \
		-L=${SHLIBDIR}
.elif ${SHLIBINSTALLDIR} != "/usr/lib"
_PROGLDOPTS+=	-Wl,-rpath-link,${DESTDIR}${SHLIBINSTALLDIR} \
		-L=${SHLIBINSTALLDIR}
.endif

__proginstall: .USE
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${STRIPFLAG} ${.ALLSRC} ${.TARGET}

__progdebuginstall: .USE
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${DEBUGOWN} -g ${DEBUGGRP} -m ${DEBUGMODE} \
		${.ALLSRC} ${.TARGET}



#
# Backwards compatibility with Makefiles that assume that bsd.prog.mk
# can only build a single binary.
#

_APPEND_MANS=yes
_APPEND_SRCS=yes

_CCLINKFLAGS=
.if defined(DESTDIR)
_CCLINKFLAGS+=	-B${_GCC_CRTDIR}/ -B${DESTDIR}/usr/lib/
.endif

.if defined(PROG_CXX)
PROG=		${PROG_CXX}
_CCLINK=	${CXX} ${_CCLINKFLAGS}
.endif

#PROGS=			${PROG}
#CPPFLAGS+=		-DCRUNCHOPS

.if defined(PROG)
_CCLINK?=	${CC} ${_CCLINKFLAGS}
.  if defined(MAN)
MAN.${PROG}=	${MAN}
_APPEND_MANS=	no
.  endif
.  if !defined(OBJS)
OBJS=		${OBJS.${PROG}}
.  endif
.  if defined(PROGNAME)
PROGNAME.${PROG}=	${PROGNAME}
.  endif
.  if defined(SRCS)
SRCS.${PROG}=	${SRCS}
_APPEND_SRCS=	no
.  endif
.endif

# Turn the single-program PROG and PROG_CXX variables into their multi-word
# counterparts, PROGS and PROGS_CXX.
.if defined(PROG_CXX) && !defined(PROGS_CXX)
PROGS_CXX=	${PROG_CXX}
.elif defined(PROG) && !defined(PROGS)
PROGS=		${PROG}
.endif

##### Libraries that this may depend upon.
.if defined(PROGDPLIBS) 						# {
.for _lib _dir in ${PROGDPLIBS}
.if !defined(BINDO.${_lib})
PROGDO.${_lib}!=	cd "${_dir}" && ${PRINTOBJDIR}
.MAKEOVERRIDES+=PROGDO.${_lib}
.endif
.if defined(PROGDPLIBSSTATIC)
DPADD+=		${PROGDO.${_lib}}/lib${_lib}.a
LDADD+=		${PROGDO.${_lib}}/lib${_lib}.a
.else
LDADD+=		-L${PROGDO.${_lib}} -l${_lib}
.if exists(${PROGDO.${_lib}}/lib${_lib}_pic.a)
DPADD+=		${PROGDO.${_lib}}/lib${_lib}_pic.a
.elif exists(${PROGDO.${_lib}}/lib${_lib}.so)
DPADD+=		${PROGDO.${_lib}}/lib${_lib}.so
.else
DPADD+=		${PROGDO.${_lib}}/lib${_lib}.a
.endif
.endif
.endfor
.endif									# }

LDADD+=${LDADD_AFTER}
DPADD+=${DPADD_AFTER}

#
# Per-program definitions and targets.
#

_CCLINK.CDEFAULT= ${CC} ${_CCLINKFLAGS}
# Definitions specific to C programs.
.for _P in ${PROGS}
SRCS.${_P}?=	${_P}.c
_CCLINK.${_P}=	${CC} ${_CCLINKFLAGS}
_CFLAGS.${_P}=	${CFLAGS} ${CPUFLAGS}
_CPPFLAGS.${_P}=	${CPPFLAGS}
_COPTS.${_P}=	${COPTS}
.endfor

_CCLINK.CXXDEFAULT= ${CXX} ${_CCLINKFLAGS}
# Definitions specific to C++ programs.
.for _P in ${PROGS_CXX}
SRCS.${_P}?=	${_P}.cc
_CCLINK.${_P}=	${CXX} ${_CCLINKFLAGS}
.endfor

# Language-independent definitions.
.for _P in ${PROGS} ${PROGS_CXX}					# {

BINDIR.${_P}?=		${BINDIR}
PROGNAME.${_P}?=	${_P}

.if ${MKDEBUG:Uno} != "no" && !defined(NODEBUG) && !commands(${_P}) && \
    empty(SRCS.${_P}:M*.sh)
_PROGDEBUG.${_P}:=	${PROGNAME.${_P}}.debug
.endif

##### PROG specific flags.

_DPADD.${_P}=		${DPADD}    ${DPADD.${_P}}
_LDADD.${_P}=		${LDADD}    ${LDADD.${_P}}
_LDFLAGS.${_P}=		${LDFLAGS}  ${LDFLAGS.${_P}}
.if ${MKSANITIZER} != "yes"
# Sanitizers don't support static build.
_LDSTATIC.${_P}=	${LDSTATIC} ${LDSTATIC.${_P}}
.endif

##### Build and install rules
.if !empty(_APPEND_SRCS:M[Yy][Ee][Ss])
SRCS+=		${SRCS.${_P}}	# For bsd.dep.mk
.endif

_YPSRCS.${_P}=	${SRCS.${_P}:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS.${_P}:M*.y:.y=.h}}

DPSRCS+=		${_YPSRCS.${_P}}
CLEANFILES+=		${_YPSRCS.${_P}}

.if !empty(SRCS.${_P}:N*.h:N*.sh:N*.fth)
OBJS.${_P}+=	${SRCS.${_P}:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
LOBJS.${_P}+=	${LSRCS:.c=.ln} ${SRCS.${_P}:M*.c:.c=.ln}
.endif

.if defined(OBJS.${_P}) && !empty(OBJS.${_P})			# {
.NOPATH: ${OBJS.${_P}} ${_P} ${_YPSRCS.${_P}}

.if (defined(USE_COMBINE) && ${USE_COMBINE} != "no" && !commands(${_P}) \
   && (${_CCLINK.${_P}} == ${_CCLINK.CDEFAULT} \
       || ${_CCLINK.${_P}} == ${_CCLINK.CXXDEFAULT}) \
   && !defined(NOCOMBINE.${_P}) && !defined(NOCOMBINE))
.for f in ${SRCS.${_P}:N*.h:N*.sh:N*.fth:C/\.[yl]$/.c/g}
#_XFLAGS.$f := ${CPPFLAGS.$f:D1} ${CPUFLAGS.$f:D2} \
#     ${COPTS.$f:D3} ${OBJCOPTS.$f:D4} ${CXXFLAGS.$f:D5}
.if (${CPPFLAGS.$f:D1} == "1" || ${CPUFLAGS.$f:D2} == "2" \
     || ${COPTS.$f:D3} == "3" || ${OBJCOPTS.$f:D4} == "4" \
     || ${CXXFLAGS.$f:D5} == "5") \
    || ("${f:M*.[cyl]}" == "" || commands(${f:R:S/$/.o/}))
XOBJS.${_P}+=	${f:R:S/$/.o/}
.else
XSRCS.${_P}+=	${f}
NODPSRCS+=	${f}
.endif
.endfor

${_P}: .gdbinit ${LIBCRT0} ${LIBCRTI} ${XOBJS.${_P}} ${SRCS.${_P}} \
    ${DPSRCS} ${LIBC} ${LIBCRTBEGIN} ${LIBCRTEND} ${_DPADD.${_P}}
	${_MKTARGET_LINK}
.if defined(DESTDIR)
	${_CCLINK.${_P}} -Wl,-nostdlib \
	    ${_LDFLAGS.${_P}} ${_LDSTATIC.${_P}} -o ${.TARGET} ${_PROGLDOPTS} \
	    -B${_GCC_CRTDIR}/ -B${DESTDIR}/usr/lib/ \
	    -MD --combine ${_CPPFLAGS.${_P}} ${_CFLAGS.${_P}} ${_COPTS.${_P}} \
	    ${XSRCS.${_P}:@.SRC.@${.ALLSRC:M*.c:M*${.SRC.}}@:O:u} ${XOBJS.${_P}} \
	    ${_LDADD.${_P}} -L${_GCC_LIBGCCDIR} -L${DESTDIR}/usr/lib
.else
	${_CCLINK.${_P}} ${_LDFLAGS.${_P}} ${_LDSTATIC.${_P}} -o ${.TARGET} ${_PROGLDOPTS} \
	    -MD --combine ${_CPPFLAGS.${_P}} ${_COPTS.${_P}}
	    ${XSRCS.${_P}:@.SRC.@${.ALLSRC:M*.c:M*${.SRC.}}@:O:u} ${XOBJS.${_P}} \
	    ${_LDADD.${_P}}
.endif	# defined(DESTDIR)
.if defined(CTFMERGE)
	${CTFMERGE} ${CTFMFLAGS} -o ${.TARGET} ${OBJS.${_P}}
.endif
.if defined(PAXCTL_FLAGS.${_P})
	${PAXCTL} ${PAXCTL_FLAGS.${_P}} ${.TARGET}
.endif
.if ${MKSTRIPIDENT} != "no"
	${OBJCOPY} -R .ident ${.TARGET}
.endif

CLEANFILES+=	${_P}.d
.if exists(${_P}.d)
.include "${_P}.d"		# include -MD depend for program.
.endif
.else	# USE_COMBINE

${OBJS.${_P}} ${LOBJS.${_P}}: ${DPSRCS}

${_P}: .gdbinit ${LIBCRT0} ${LIBCRTI} ${OBJS.${_P}} ${LIBC} ${LIBCRTBEGIN} \
    ${LIBCRTEND} ${_DPADD.${_P}}
.if !commands(${_P})
	${_MKTARGET_LINK}
	${_CCLINK.${_P}} \
	    ${_LDFLAGS.${_P}} ${_LDSTATIC.${_P}} -o ${.TARGET} \
	    ${OBJS.${_P}} ${_PROGLDOPTS} ${_LDADD.${_P}}
.if defined(CTFMERGE)
	${CTFMERGE} ${CTFMFLAGS} -o ${.TARGET} ${OBJS.${_P}}
.endif
.if defined(PAXCTL_FLAGS.${_P})
	${PAXCTL} ${PAXCTL_FLAGS.${_P}} ${.TARGET}
.endif
.if ${MKSTRIPIDENT} != "no"
	${OBJCOPY} -R .ident ${.TARGET}
.endif
.endif	# !commands(${_P})
.endif	# USE_COMBINE

${_P}.ro: ${OBJS.${_P}} ${_DPADD.${_P}}
	${_MKTARGET_LINK}
	${CC} ${LDFLAGS:N-pie} -nostdlib -r -Wl,-dc -o ${.TARGET} ${OBJS.${_P}}

.if defined(_PROGDEBUG.${_P})
${_PROGDEBUG.${_P}}: ${_P}
	${_MKTARGET_CREATE}
	(  ${OBJCOPY} --only-keep-debug ${_P} ${_PROGDEBUG.${_P}} \
	&& ${OBJCOPY} --strip-debug -p -R .gnu_debuglink \
		--add-gnu-debuglink=${_PROGDEBUG.${_P}} ${_P} \
	) || (rm -f ${_PROGDEBUG.${_P}}; false)
.endif

.endif	# defined(OBJS.${_P}) && !empty(OBJS.${_P})			# }

.if !defined(MAN.${_P})
MAN.${_P}=	${_P}.1
.endif	# !defined(MAN.${_P})
.if !empty(_APPEND_MANS:M[Yy][Ee][Ss])
MAN+=		${MAN.${_P}}
.endif

realall: ${_P} ${_PROGDEBUG.${_P}}

CLEANFILES+= ${_P} ${_PROGDEBUG.${_P}}

.if defined(OBJS.${_P}) && !empty(OBJS.${_P})
CLEANFILES+= ${OBJS.${_P}} ${LOBJS.${_P}}
.endif

_PROG_INSTALL+=	proginstall-${_P}

.if !target(proginstall-${_P})						# {
proginstall-${_P}::	${DESTDIR}${BINDIR.${_P}}/${PROGNAME.${_P}} \
		${_PROGDEBUG.${_P}:D${DESTDIR}${DEBUGDIR}${BINDIR.${_P}}/${_PROGDEBUG.${_P}}}
.PRECIOUS:	${DESTDIR}${BINDIR.${_P}}/${PROGNAME.${_P}} \
		${_PROGDEBUG.${_P}:D${DESTDIR}${DEBUGDIR}${BINDIR.${_P}}/${_PROGDEBUG.${_P}}}

.if ${MKUPDATE} == "no"
${DESTDIR}${BINDIR.${_P}}/${PROGNAME.${_P}}! ${_P} __proginstall
.if !defined(BUILD) && !make(all) && !make(${_P})
${DESTDIR}${BINDIR.${_P}}/${PROGNAME.${_P}}! .MADE
.endif
.if defined(_PROGDEBUG.${_P})
${DESTDIR}${DEBUGDIR}${BINDIR.${_P}}/${_PROGDEBUG.${_P}}! ${_PROGDEBUG.${_P}} __progdebuginstall
.if !defined(BUILD) && !make(all) && !make(${_P})
${DESTDIR}${DEBUGDIR}${BINDIR.${_P}}/${_PROGDEBUG.${_P}}! .MADE
.endif
.endif	#  define(_PROGDEBUG.${_P})
.else	# MKUPDATE != no
${DESTDIR}${BINDIR.${_P}}/${PROGNAME.${_P}}: ${_P} __proginstall
.if !defined(BUILD) && !make(all) && !make(${_P})
${DESTDIR}${BINDIR.${_P}}/${PROGNAME.${_P}}: .MADE
.endif
.if defined(_PROGDEBUG.${_P})
${DESTDIR}${DEBUGDIR}${BINDIR.${_P}}/${_PROGDEBUG.${_P}}: ${_PROGDEBUG.${_P}} __progdebuginstall
.if !defined(BUILD) && !make(all) && !make(${_P})
${DESTDIR}${DEBUGDIR}${BINDIR.${_P}}/${_PROGDEBUG.${_P}}: .MADE
.endif
.endif	#  defined(_PROGDEBUG.${_P})
.endif	# MKUPDATE != no

.endif	# !target(proginstall-${_P})					# }

lint: lint-${_P}
lint-${_P}: ${LOBJS.${_P}}
.if defined(LOBJS.${_P}) && !empty(LOBJS.${_P})
.if defined(DESTDIR)
	${LINT} ${LINTFLAGS} ${_LDFLAGS.${_P}:C/-L[  ]*/-L/Wg:M-L*} -L${DESTDIR}/usr/libdata/lint ${LOBJS.${_P}} ${_LDADD.${_P}}
.else
	${LINT} ${LINTFLAGS} ${_LDFLAGS.${_P}:C/-L[  ]*/-L/Wg:M-L*} ${LOBJS.${_P}} ${_LDADD.${_P}}
.endif
.endif

.endfor # _P in ${PROGS} ${PROGS_CXX}					# }

.if defined(OBJS) && !empty(OBJS) && \
    (empty(PROGS) && empty(PROGS_CXX))
CLEANFILES+= ${OBJS} ${LOBJS}
.endif

.if !target(proginstall)
proginstall:: ${_PROG_INSTALL}
.endif
.PHONY:		proginstall



realall: ${SCRIPTS}
.if defined(SCRIPTS) && !target(scriptsinstall)				# {
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
.endif									# }

.if !target(scriptsinstall)
scriptsinstall::
.endif
.PHONY:		scriptsinstall

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
