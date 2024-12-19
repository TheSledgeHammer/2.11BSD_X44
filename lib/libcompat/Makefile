#	$NetBSD: Makefile,v 1.32 2013/02/07 20:15:07 christos Exp $

.include <bsd.own.mk>

WARNS=	2

LIB=	compat
CPPFLAGS+=-DLIBC_SCCS -DSYSLIBC_SCCS
AFLAGS+=-I${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_ARCH}
.if defined(DESTDIR)
AFLAGS+=-nostdinc -isystem ${DESTDIR}/usr/include
.endif

.PATH:	${.CURDIR}/4.1/${MACHINE_ARCH} ${.CURDIR}/4.1 \
		${.CURDIR}/4.3/${MACHINE_ARCH} ${.CURDIR}/4.3 \
		${.CURDIR}/4.4/${MACHINE_ARCH} ${.CURDIR}/4.4 \
		${.CURDIR}/regexp

# compat 4.1 sources
# missing: ftime.c getpw.c gtty.c stty.c tell.c vlimit.c vtimes.c
#SRCS=	ftime.c getpw.c gtty.c stty.c tell.c vlimit.c vtimes.c

.if (${MACHINE_ARCH} == "vax")
SRCS+=	reset.s
.endif

MAN+=	ftime.3 getpw.3 stty.3 vlimit.3 vtimes.3

MLINKS+= stty.3 gtty.3

# compat 4.3 sources
# missing: ecvt.c gcvt.c regex.c sibuf.c sobuf.c strout.c
SRCS+=	cfree.c lsearch.c rexec.c

.if	(${MACHINE_ARCH} == "hp300" || ${MACHINE_ARCH} == "luna68k")
SRCS+=	insque.s remque.s
.elif	(${MACHINE_ARCH} == "i386")
SRCS+=	insque.c remque.c
.elif	(${MACHINE_ARCH} == "mips")
SRCS+=	insque.c remque.c
.elif	(${MACHINE_ARCH} == "sparc")
SRCS+=	insque.c remque.c
.elif	(${MACHINE_ARCH} == "tahoe")
SRCS+=	nargs.s insque.s remque.s
.elif	(${MACHINE_ARCH} == "vax")
SRCS+=	nargs.s insque.s remque.s
.endif

MAN+=	ecvt.3 insque.3 lsearch.3 re_comp.3 rexec.3

MLINKS+=ecvt.3 fcvt.3 ecvt.3 gcvt.3
MLINKS+=insque.3 remque.3
MLINKS+=re_comp.3 re_exec.3

# compat 4.4 sources
SRCS+=	cuserid.c

# regexp sources
SRCS+=	regerror.c regexp.c regsub.c

MAN+=	regexp.3

MLINKS+=regexp.3 regcomp.3 regexp.3 regexec.3 regexp.3 regsub.3 \
		regexp.3 regerror.3

.include <bsd.lib.mk>
