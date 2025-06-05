#	$NetBSD: Makefile.inc,v 1.15 2024/06/07 20:50:13 riastradh Exp $

# The current iteration of gdtoa is a refactored version of stdlib/strtod.c (NetBSD 3.1).
# As well as support for integer, floating and extended precision (NetBSD 4.0 / 5.2),
# and includes ieee infinity.
# Note: Floating and extended precision are WIP.

# gdtoa sources
.PATH: ${.CURDIR}/gdtoa
CPPFLAGS+=-I${.CURDIR}/gdtoa -I${.CURDIR}/locale

# ieee754 infinity constants and functions
.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "mips" || \
    ${MACHINE_ARCH} == "riscv"
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_ieee754.c
.endif

.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "arm" || \
    ${MACHINE_ARCH} == "hppa" || ${MACHINE_ARCH} == "ia64" || \
	${MACHINE_ARCH} == "or1k" || ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "powerpc64" || ${MACHINE_ARCH} == "sh3" || \
    ${MACHINE_ARCH} == "sparc"
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
.endif

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "vax" || \
    ${MACHINE_ARCH} == "x86_64"
SRCS+=  infinityl.c
.endif

.if ${MACHINE_ARCH} == "vax"
SRCS+=  infinityf.c infinity.c
.endif

.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "hppa" || \
	${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "m68k" || \
    ${MACHINE_ARCH} == "mips" || ${MACHINE_ARCH} == "vax" || \
    ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "powerpc64" || \
    ${MACHINE_ARCH} == "riscv" || ${MACHINE_ARCH} == "sh3" || \
    ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "x86_64"
SRCS+=  nanf.c
.endif

# machine-dependent Makefile must define names of modules to use for
# float conversions:
# 	strtof(): strtof{,_vaxf}.c
# and for long double conversions on platforms with extended-precision
# format, if supported (strtod.c will define an alias otherwise):
#	strtold(): strtold_p{x,xL,Q}.c
#	helpers:   strtop{x,xL,Q}.c

.if ${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH} == "arm" || \
    ${MACHINE_ARCH} == "hppa" || ${MACHINE_ARCH} == "ia64" || \
	${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "m68k" || \
    ${MACHINE_ARCH} == "mips" || ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "powerpc64" || ${MACHINE_ARCH} == "sh3" || \
    ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64" || \
    ${MACHINE_ARCH} == "x86_64"
SRCS+= strtof.c
.endif

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS+= 	strtold.c
SRCS+= 	strtopx.c
.endif

# public interfaces
SRCS+=	strtod.c

# private interfaces interfacing to libc
SRCS+=	dtoa.c

# private interfaces
SRCS+= 	dmisc.c \
		gethex.c gmisc.c \
		hd_init.c hexnan.c \
		misc.c \
		smisc.c strtodg.c sum.c

# XXX no hexadecimal floating-point string conversion for VAX FP yet
.if ${MACHINE_ARCH} != "vax"
SRCS+=	strtord.c
.endif
