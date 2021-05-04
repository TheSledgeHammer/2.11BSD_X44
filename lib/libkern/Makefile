#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1996/3/26
#
# This is the Makefile for 'libkern.a'.  These are the specially compiled
# versions of libc.a routines which the kernel and network use.
#
# There are no source files in this directory.  Links are made to the files.
#
# libkern.a needs to be installed only once (usually when the 
# C library is installed) rather than compiling the sources each time 
# a kernel is compiled..

DEFS=	-DKERNEL
CFLAGS=	 -O ${DEFS} -I../libc/arch/${MACHINE}/gen
CPP=		/lib/cpp -P ${DEFS} -I../libc/arch/${MACHINE}/gen
AS=		/bin/as -u
PREFIX=	/usr/src/lib/libc/arch/${MACHINE}

CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S} -I${S}/arch

CPPFLAGS+= -D_STANDALONE

COPTS+=	-ffreestanding
COPTS+=	-fno-stack-protector
COPTS+=	-fno-unwind-tables
CWARNFLAGS+= -Werror
CWARNFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
CPPFLAGS+= -D_KERNTYPES

### find out what to use for libkern
KERN_AS=	library
KERNDIR=	${S}/lib/libkern

.include "${KERNDIR}/Makefile.libkern"

KQSRCS=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
		lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
		subdi3.c ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
KSRCS=	bcmp.c ffs.c index.c mcount.c rindex.c strcat.c strcmp.c strcpy.c \
		strlen.c strncpy.c
		
libkern: libkern.gen libkern.${MACHINE}

libkern.gen: ${KQSRCS} ${KSRCS}
	cp -p ${.CURDIR}/quad/quad.h ${.ALLSRC} /sys/libkern

libkern.${MACHINE}:: ${KMSRCS}
.if defined(KMSRCS) && !empty(KMSRCS)
	cp -p ${.ALLSRC} /sys/libkern/${MACHINE}
.endif

.include <bsd.lib.mk>