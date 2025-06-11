#	$NetBSD: Makefile.inc,v 1.1.2.2 1999/04/04 07:28:05 tsubai Exp $

.include <bsd.own.mk>

PICFLAGS?=	-fPIC

ARCH_DIR:=		${ARCHDIR}
COMMON_DIR:=	${.CURDIR}/common
.PATH:		    ${COMMON_DIR}

#CPPFLAGS+=	-DPIC -DDYNAMIC
CPPFLAGS+=	-I${ARCH_DIR} -I.
CPPFLAGS+=	-I${COMMON_DIR} -I.
CPPFLAGS+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/dlfcn
CPPFLAGS+=  	-DJCR

OBJS+= crt0.o gcrt0.o
OBJS+= crtbegin.o crtend.o
OBJS+= crti.o crtn.o

.if ${MKPIC} == "yes"
OBJS+=		crtbeginS.o
OBJS+=		crtendS.o
CFLAGS.crtbegin.c+= -fPIE
CFLAGS.crtend.c+= -fPIE
.endif

realall: ${OBJS}

crtbegin.o: crtbegin.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crtbegin.c} ${COMMON_DIR}/crtbegin.c -o ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

crtend.o: crtend.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crtend.c} ${COMMON_DIR}/crtend.c -o ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

crtbeginS.o: crtbegin.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crtbegin.c} ${PICFLAGS} -DSHARED ${COMMON_DIR}/crtbegin.c -o ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

crtendS.o: crtend.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crtend.c} ${PICFLAGS} -DSHARED ${COMMON_DIR}/crtend.c -o ${.TARGET}.o
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

crt0.o: crt0.S crt0_common.c
		${_MKTARGET_COMPILE}
		${COMPILE.S} ${ARCH_DIR}/crt0.S -o ${.TARGET}.S.o
		${COMPILE.c} ${CFLAGS.crt0_common.c} ${MY_PICFLAGS} ${COMMON_DIR}/crt0_common.c -o ${.TARGET}.c.o
		${LD} -r -o ${.TARGET}.o ${.TARGET}.S.o ${.TARGET}.c.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.S.o ${.TARGET}.c.o ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

gcrt0.o: crt0.S crt0_common.c
		${_MKTARGET_COMPILE}
		${COMPILE.S} ${ARCH_DIR}/crt0.S -o ${.TARGET}.S.o
		${COMPILE.c} ${MY_PICFLAGS} -DMCRT0 ${COMMON_DIR}/crt0_common.c -o ${.TARGET}.c.o
		${LD} -r -o ${.TARGET}.o ${.TARGET}.S.o ${.TARGET}.c.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.S.o ${.TARGET}.c.o ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

crti.o: 	crti.c elf_note.h
crtn.o: 	crtn.c

FILES=${OBJS}
FILESDIR=${LIBDIR}
CLEANFILES+=${OBJS}

.if ${MKPIC} == "yes"
SYMLINKS+=	crtbegin.o ${LIBDIR}/crtbeginT.o
.endif

.include <bsd.prog.mk>
