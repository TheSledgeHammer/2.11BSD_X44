#	$NetBSD: Makefile.inc,v 1.15 2024/06/07 20:50:13 riastradh Exp $

# The current iteration of gdtoa is a refactored version of stdlib/strtod.c (NetBSD 3.1).
# As well as support for integer, floating and extended precision (NetBSD 4.0 / 5.2),
# and includes ieee infinity.
# Note: Floating and extended precision are WIP.

# gdtoa sources
.PATH: ${.CURDIR}/gdtoa
CPPFLAGS+=-I${.CURDIR}/gdtoa -I${.CURDIR}/locale

# ieee754 infinity constants and functions
.if ${MACHINE_ARCH} == __aarch64__ || __mips__ || __riscv__
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_ieee754.c
.endif

.if ${MACHINE_ARCH} == __alpha__ || __arm__ || __hppa__ || __ia64__ || \
	__or1k__ || __powerpc__ || __powerpc64__ || __sh3__ || __sparc__
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
.endif

.if ${MACHINE_ARCH} == __i386__ || __vax__ || __x86_64__
SRCS+=  infinityl.c
.endif

.if ${MACHINE_ARCH} == __vax__
SRCS+=  infinityf.c infinity.c
.endif

.if ${MACHINE_ARCH} == __aarch64__ || __alpha__ || __arm__ || __hppa__ || \
	__i386__ || __m68k__ ||  __mips__ || __vax__ || __powerpc__ || \
    __powerpc64__ || __riscv__ ||  __sh3__ || __sparc__ || __x86_64__
SRCS+=  nanf.c
.endif

# machine-dependent Makefile must define names of modules to use for
# float conversions:
# 	strtof(): strtof{,_vaxf}.c
# and for long double conversions on platforms with extended-precision
# format, if supported (strtod.c will define an alias otherwise):
#	strtold(): strtold_p{x,xL,Q}.c
#	helpers:   strtop{x,xL,Q}.c


.if ${MACHINE_ARCH} == __alpha__ || __arm__ || __hppa__ || __ia64__ || \
	__i386__ || __m68k__ || __mips__ || __powerpc__ || __powerpc64__ || \
	__sh3__ || __sparc__ || __sparc64 || __x86_64__
SRCS+= strtof.c
.endif

.if ${MACHINE_ARCH} == __i386__ || __x86_64__
#SRCS+= strtold_px.c
#SRCS+= strtopx.c
.endif

# public interfaces
SRCS+=	strtod.c

# private interfaces interfacing to libc
SRCS+=	dtoa.c

# private interfaces
SRCS+= 	gethex.c gmisc.c hd_init.c hexnan.c \
		misc.c smisc.c strtodg.c sum.c

# XXX no hexadecimal floating-point string conversion for VAX FP yet
.if ${MACHINE_ARCH} != __vax__
SRCS+=	strtord.c
.endif
