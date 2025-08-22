#	$NetBSD: Makefile,v 1.28 2014/03/18 18:20:39 riastradh Exp $

.include <bsd.own.mk>   # For MKPIC
.include <bsd.shlib.mk>
.include <bsd.endian.mk>
 
.if ${MKPIC} != "no" && \
    (${OBJECT_FMT} == "a.out" ||	\
    (${OBJECT_FMT} == "ELF" &&		\
    ((${MACHINE_CPU} == "arm" && 	\
    ${TARGET_ENDIANNESS} == "1234") ||	\
     ${MACHINE_ARCH} == "i386" ||	\
     ${MACHINE_CPU} == "m68k" ||	\
     ${MACHINE_ARCH} == "sparc" ||	\
     ${MACHINE_ARCH} == "vax")))

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

CPPFLAGS += -I${NETBSDSRCDIR}/libexec/ld.aout_so
.PATH:	${NETBSDSRCDIR}/libexec/ld.aout_so

.if ${SHLIBINSTALLDIR} != ${LIBDIR}
CPPFLAGS+=-DSTANDARD_SEARCH_DIRS=\"${SHLIBINSTALLDIR}\",\"${LIBDIR}\"
.endif

.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
