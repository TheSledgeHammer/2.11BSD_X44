#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

SRCS+=	alloca.S fabs.S modf.S
SRCS+=	setjmp.S _setjmp.S
SRCS+=	makecontext.c
SRCS+=	isinf.c ldexp.c
# "internal" objects (don't provide part of the user-visible API)
SRCS+=	divsi3.S fixdfsi.S udivsi3.S
# cannot compile, text relocation error
#fixunsdfsi.S resumecontext.S swapcontext.S
