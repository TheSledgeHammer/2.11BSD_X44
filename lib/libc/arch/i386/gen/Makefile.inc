#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

SRCS+=	alloca.S fabs.S modf.S
SRCS+=	setjmp.S _setjmp.S
SRCS+=  sigsetjmp.S
SRCS+=	makecontext.c resumecontext.S swapcontext.S
SRCS+=	isinf.c ldexp.c
# "internal" objects (don't provide part of the user-visible API)
SRCS+=	divsi3.S fixdfsi.S fixunsdfsi.S udivsi3.S
