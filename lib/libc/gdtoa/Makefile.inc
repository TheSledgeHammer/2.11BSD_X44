#	$NetBSD: Makefile.inc,v 1.15 2024/06/07 20:50:13 riastradh Exp $

# The current iteration of gdtoa is a refactored version of stdlib/strtod.c (NetBSD 4.0).
# With a few additions such as ieee infinity.
# Currently untested since refactored and has not been added to the libc Makefile.inc

# gdtoa sources
.PATH: ${.CURDIR}/gdtoa
CPPFLAGS+=-I${.CURDIR}/gdtoa -I${.CURDIR}/locale

# ieee754 infinity constants and functions
.if ${MACHINE_ARCH} == "aarch64" || "mips" || "riscv"
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_ieee754.c
.endif
.if  ${MACHINE_ARCH} == "alpha" || "arm" || "hppa" || \
	"ia64" || "or1k" || "powerpc" || \
	"powerpc64" || "sh3" || "sparc"
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
.endif

# machine-dependent Makefile must define names of modules to use for
# float conversions:
# 	strtof(): strtof{,_vaxf}.c
# and for long double conversions on platforms with extended-precision
# format, if supported (strtod.c will define an alias otherwise):
#	strtold(): strtold_p{x,xL,Q}.c
#	helpers:   strtop{x,xL,Q}.c

# public interfaces
SRCS+=	strtod.c

# private interfaces interfacing to libc
SRCS+=	dtoa.c

# private interfaces
SRCS+= 	misc.c
