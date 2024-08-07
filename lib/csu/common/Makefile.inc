#	$NetBSD: Makefile.inc,v 1.1.2.2 1999/04/04 07:28:05 tsubai Exp $

.include <bsd.own.mk>

.include "../../Makefile.inc"

COMMON_DIR:= ${.CURDIR}/../common

PICFLAGS=	-fPIC
CPPFLAGS+=	-DDYNAMIC
CPPFLAGS+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+=	-I${COMMON_DIR}
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/dlfcn


.PATH:		${COMMON_DIR}

OBJS+=	crt0.o gcrt0.o crtbegin.o crtend.o crti.o crtn.o elf_note.o

all: ${OBJS}

crtbegin.o: crtbegin.c crtbegin.h
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crtbegin.c} ${COMMON_DIR}/crtbegin.c -o ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

crtend.o: crtend.c crtend.h
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crtend.c} ${COMMON_DIR}/crtend.c -o ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

.if ${MKPIC} != "no"
MY_PICFLAGS=	${PICFLAGS}
.else
MY_PICFLAGS=
.endif

crt0.o: crt0.c crt0_common.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crt0.c} ${MY_PICFLAGS} ${ARCHDIR}/crt0.c -o ${.TARGET}.o
		${COMPILE.c} ${CFLAGS.crt0_common.c} ${MY_PICFLAGS} ${COMMON_DIR}/crt0_common.c -o ${.TARGET}.o
		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

# dependant on crt0.o to pick up header dependencies
gcrt0.o: crt0.c crt0_common.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${MY_PICFLAGS} -DMCRT0 ${ARCHDIR}/crt0.c -o ${.TARGET}.o
		${COMPILE.c} ${MY_PICFLAGS} -DMCRT0 ${COMMON_DIR}/crt0_common.c -o ${.TARGET}.o
		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

elf_note.o: elf_note.S
		${_MKTARGET_COMPILE}
		${COMPILE.S} elf_note.S -o ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o

crti.o: crti.S
crtn.o: crtn.S
elf_note.o: elf_note.S elf_note.h

FILES=${OBJS}
FILESDIR=${LIBDIR}
CLEANFILES+=${OBJS}

.include <bsd.prog.mk>
