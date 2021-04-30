#	$NetBSD: Makefile,v 1.36.8.1 1997/11/12 23:51:52 thorpej Exp $

LIB=		kern
NOPIC=		# defined
LLIBS=		# defined

.include <bsd.own.mk>

.if defined(LIBKERN_ARCH) && !empty(LIBKERN_ARCH) && \
    exists(${KERNDIR}/arch/${LIBKERN_ARCH})
ARCHSUBDIR=     ${LIBKERN_ARCH}
.elif defined(MACHINE_ARCH) && !empty(MACHINE_ARCH) && \
    exists(${KERNDIR}/arch/${MACHINE_ARCH})
ARCHSUBDIR=     ${MACHINE_ARCH}
.elif defined(MACHINE_CPU) && !empty(MACHINE_CPU) && \
    exists(${KERNDIR}/arch/${MACHINE_CPU}) 
ARCHSUBDIR=     ${MACHINE_CPU}
.else
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

M=			${.CURDIR}/arch/${MACHINE_ARCH}

CPPFLAGS=	-I$M ${KERNCPPFLAGS} ${KERNMISCCPPFLAGS}

.if exists ($M/Makefile.inc)
.PATH:	$M
.include "$M/Makefile.inc"
.endif

.if (${MACHINE_ARCH} != "alpha")
# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
.endif

# Other stuff
SRCS+=	bcd.c inet_addr.c intoa.c pmatch.c
SRCS+=	arc4random.c murmur3_32.c prospectorhash.c

SRCS+=	strstr.c strlcpy.c strlcat.c

# Files to clean up
CLEANFILES+= lib${LIB}.o lib${LIB}.po

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po

# only needed during build
libinstall::

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `NM=${NM} ${LORDER} ${OBJS} | ${TSORT}`

POBJS+=	${OBJS:.o=.po} mcount.po
lib${LIB}.po:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}.po
	@${LD} -r -o lib${LIB}.po `NM=${NM} ${LORDER} ${POBJS} | ${TSORT}`