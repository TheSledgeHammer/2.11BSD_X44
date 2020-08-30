#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	4.19.2 (2.11BSD) 1997/8/28
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

LIB=c
CPPFLAGS+=	-I${.CURDIR}/include -I${.CURDIR}
AINC=		-I${.CURDIR}/${MACHINE}
CLEANFILES+=tags

.include "${.CURDIR}/db/Makefile.inc"
.include "${.CURDIR}/dlfcn/Makefile.inc"
.include "${.CURDIR}/compat-4.1/Makefile.inc"
.include "${.CURDIR}/compat-43/Makefile.inc"
.include "${.CURDIR}/compat-sys5/Makefile.inc"
.include "${.CURDIR}/gen/Makefile.inc"
.include "${.CURDIR}/gmon/Makefile.inc"
.include "${.CURDIR}/inet/Makefile.inc"
.include "${.CURDIR}/locale/Makefile.inc"
.include "${.CURDIR}/net/Makefile.inc"
.include "${.CURDIR}/ns/Makefile.inc"
.include "${.CURDIR}/quad/Makefile.inc"
.include "${.CURDIR}/regex/Makefile.inc"
.include "${.CURDIR}/stdio/Makefile.inc"
.include "${.CURDIR}/stdlib/Makefile.inc"
.include "${.CURDIR}/string/Makefile.inc"
.include "${.CURDIR}/termios/Makefile.inc"

beforeinstall: tags
	install -c -o bin -g bin -m 444 tags /var/db/libc.tags

tags: ${SRCS}
	ctags ${.ALLSRC:M*.c}
	egrep -o "^ENTRY(.*)|^FUNC(.*)|^SYSCALL(.*)" ${.ALLSRC:M*.s} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> tags; sort -o tags tags

.include <bsd.lib.mk>

# force the dynamic linker to initialize libc first
SHLIB_SHFLAGS+=	-Wl,-z,initfirst
.if ${HAVE_LIBGCC} == "no"
SHLIB_SHFLAGS+= -Wl,-z,defs
.endif