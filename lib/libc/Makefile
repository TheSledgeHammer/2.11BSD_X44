#	$NetBSD: Makefile,v 1.173 2020/05/16 18:31:47 christos Exp $
#	@(#)Makefile	4.19.2 (2.11BSD) 1997/8/28
#	@(#)Makefile	8.2 (Berkeley) 2/3/94
#
# All library objects contain sccsid strings by default; they may be
# excluded as a space-saving measure.  To produce a library that does
# not contain these strings, delete -DLIBC_SCCS and -DSYSLIBC_SCCS
# from CPPFLAGS below.  To remove these strings from just the system call
# stubs, remove just -DSYSLIBC_SCCS from CPPFLAGS.
#
# The NLS (message catalog) functions are always in libc.  To choose that
# strerror(), perror(), strsignal(), psignal(), etc. actually call the NLS
# functions, put -DNLS on the CPPFLAGS line below.
#
# The YP functions are always in libc. To choose that getpwent() and friends
# actually call the YP functions, put -DYP on the CPPFLAGS line below.
#
# The Hesiod functions are always in libc. To choose that getpwent() and friends
# actually call the Hesiod functions, put -DHESIOD on the CPPFLAGS line below.
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# The system has two monitoring schemes, utilizing either ``prof''
# or ``gprof''. DFLMON determines the default profiler; mon.o
# specifies that ``prof'' is the default, gmon.o specifies that
# ``gprof'' is the default.
#
# Machine dependent routines are located in a subtree which parallels
# the top directories.  This subtree is identified by the machine name.
#
# Compatibility routines are kept in directories with a prefixing
# ``compat'' (so they all sort together).
# 
# The C run-time startup code is always machine dependent and expected
# to be located in ${MACHINE}/csu
#
# All files contain sccsid strings, but these are not compiled into
# library objects by default, as a space-saving measure.  To produce
# a library that contains these strings in every object except
# system call stubs, add -DLIBC_SCCS to DEFS below; to put these
# strings into system call stubs, use -DSYSLIBC_SCCS.
#
# To compile a non-floating point versions of some standard library
# routines add -DNONFP.  This will speed up some operations if you don't
# have hardware floating point.  To compile a non-separate I&D version add
# -DNONSEPARATE.
#
# There are two possible methods of doing host name look up.  They are
# to use the name server or /etc/hosts. HOSTLOOKUP defines which method
# is to be compiled into libc  :
#								     
#	defining HOSTLOOKUP to be "named", compiles the         
#	host lookup routines that use the BIND name server.
#
#	defining HOSTLOOKUP to be "hosttable", compiles
#	the host lookup routines that use /etc/hosts.
#

# generated code depends on RTTI
LIBCSANITIZERFLAGS+=	${${ACTIVE_CC} == "clang":? -fno-sanitize=function :}

.include "Makefile.inc"

LIB=		c
CPPFLAGS+=	-I${.CURDIR}/include -I${.CURDIR}

LIBCDIR=	${.CURDIR}

.if exists (${ARCHDIR}/Makefile.inc)
.PATH:	${ARCHDIR}
.include "${ARCHDIR}/Makefile.inc"
.endif

.if exists (${ARCHDIR}/genassym.cf)
DPSRCS+=		assym.h
CLEANFILES+=	assym.h assym.h.tmp

assym.h: ${ARCHDIR}/genassym.cf
	${_MKTARGET_CREATE}
	${TOOL_GENASSYM} -- ${CC} ${CFLAGS:N-Wa,*} \
		${CPPFLAGS} ${CPPFLAGS.assym.h} ${PROF} \
		${GENASSYM_CPPFLAGS} < ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
.endif

#.include "${.CURDIR}/atomic/Makefile.inc"
.include "${.CURDIR}/cdb/Makefile.inc"
.include "${.CURDIR}/citrus/Makefile.inc"
.include "${.CURDIR}/compat-4.1/Makefile.inc"
.include "${.CURDIR}/compat-4.3/Makefile.inc"
.include "${.CURDIR}/compat-sys5/Makefile.inc"
.include "${.CURDIR}/db/Makefile.inc"
.include "${.CURDIR}/dlfcn/Makefile.inc"
.include "${.CURDIR}/gdtoa/Makefile.inc"
.include "${.CURDIR}/gen/Makefile.inc"
.include "${.CURDIR}/gmon/Makefile.inc"
.include "${.CURDIR}/hash/Makefile.inc"
.include "${.CURDIR}/iconv/Makefile.inc"
.include "${.CURDIR}/inet/Makefile.inc"
.include "${.CURDIR}/locale/Makefile.inc"
.include "${.CURDIR}/net/Makefile.inc"
.include "${.CURDIR}/nls/Makefile.inc"
.include "${.CURDIR}/nss/Makefile.inc"
.include "${.CURDIR}/quad/Makefile.inc"
.include "${.CURDIR}/regex/Makefile.inc"
.include "${.CURDIR}/resolv/Makefile.inc"
.include "${.CURDIR}/rpc/Makefile.inc"
.include "${.CURDIR}/ssp/Makefile.inc"
.include "${.CURDIR}/stdio/Makefile.inc"
.include "${.CURDIR}/stdlib/Makefile.inc"
.include "${.CURDIR}/string/Makefile.inc"
.include "${.CURDIR}/sys/Makefile.inc"
.include "${.CURDIR}/termios/Makefile.inc"
.include "${.CURDIR}/thread/Makefile.inc"
.include "${.CURDIR}/time/Makefile.inc"
.include "${.CURDIR}/uuid/Makefile.inc"

.if (${MKYP} != "no")
.include "${.CURDIR}/yp/Makefile.inc"
.endif

# Remove from SRCS the .c files for any .S files added by the MD makefiles,
# also remove from SRCS the .c files for the .S and .c files in NO_SRCS.
# Add the .c file for .S files (in both variables) to LSRCS so that the
# 'normal' .c file for assembly files is used for the lint librray.
#
# Usage:
# Add .S files to NO_SRSC when another .S file provides the entry points.
# Add .c files to NO_SRSC when another .c file provides the entry points.
# (lint is run on all .c files in SRCS)

.for check_file in ${SRCS:M*.S} ${NO_SRCS}
unwanted_file := ${SRCS:M${check_file:.S=.c}}
.if "${unwanted_file}" != ""
SRCS := ${SRCS:N${unwanted_file}}
.if "${unwanted_file}" != "${check_file}"
LSRCS := ${LSRCS} ${unwanted_file}
.endif
.endif
.endfor

NLS=	C.msg Pig.msg ca.msg cs.msg de.msg es.msg fi.msg fr.msg nl.msg \
	no.msg pl.msg sk.msg sv.msg

.if ${MKREPRO:Uno} == "yes"
REGEX_SPECIALS=[][)(^$$.?*\\;]
MKREPRO_SED=   -e 's;${NETBSDSRCDIR:C/${REGEX_SPECIALS}/\\\\&/g};/usr/src;'
.endif

.if !defined(MLIBDIR)
realall: tags
tags: ${SRCS}
	${_MKTARGET_CREATE}
	-${TOOL_CTAGS} -f ${.TARGET}.tmp -w ${.ALLSRC:M*.c}
	-egrep "^ENTRY(.*)|^FUNC(.*)|^SYSCALL(.*)" /dev/null ${.ALLSRC:M*.S} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
           >> ${.TARGET}.tmp
.if ${MKREPRO:Uno} == "yes"
	sed ${MKREPRO_SED} <${.TARGET}.tmp | sort -o ${.TARGET}
.else
	sort -o ${.TARGET} ${.TARGET}.tmp
.endif
	rm -f ${.TARGET}.tmp

FILES=		tags
FILESNAME=	libc.tags
FILESDIR=	/var/db
.endif

# workaround for I18N stuffs: build singlebyte setlocale() for libc.a,
# multibyte for libc.so.  the quirk should be removed when we support
# dlopen() from within statically linked binaries.
CSHLIBFLAGS+=	-D_I18N_DYNAMIC

.include <bsd.lib.mk>

# force the dynamic linker to initialize libc first
SHLIB_SHFLAGS+=	-Wl,-z,initfirst
.if ${HAVE_LIBGCC} == "no"
SHLIB_SHFLAGS+= -Wl,-z,defs
.endif
