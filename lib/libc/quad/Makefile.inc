#	$NetBSD: Makefile.inc,v 1.10 2001/05/18 15:23:44 fredette Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c fixdfdi.c \
		fixsfdi.c fixunsdfdi.c fixunssfdi.c floatdidf.c floatdisf.c \
		floatunsdidf.c iordi3.c lshldi3.c lshrdi3.c moddi3.c muldi3.c \
		negdi2.c notdi2.c qdivrem.c subdi3.c ucmpdi2.c udivdi3.c umoddi3.c \
		xordi3.c

.if   (${MACHINE_ARCH} == "m68k")
SRCS+=	ashldi3.S ashrdi3.S lshrdi3.S
.elif (${MACHINE_ARCH} == "m68000")
SRCS+=	ashldi3.S lshrdi3.S
.else
SRCS+=	ashldi3.c ashrdi3.c lshrdi3.c
.endif

.PATH: ${ARCHDIR}/quad ${.CURDIR}/quad
