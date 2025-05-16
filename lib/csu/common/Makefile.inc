#	$NetBSD: Makefile.inc,v 1.1.2.2 1999/04/04 07:28:05 tsubai Exp $

.include <bsd.own.mk>

PICFLAGS?=	-fPIC

COMMON_DIR:=	${.CURDIR}/common
.PATH:		    ${COMMON_DIR}

SHAREDFLAGS= -DSHARED

#CPPFLAGS+=	-DPIC -DDYNAMIC
CPPFLAGS+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+=	-I${COMMON_DIR} -I.
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/dlfcn
CPPFLAGS+=  -DJCR

OBJS+= crt0.o gcrt0.o
OBJS+= crtbegin.o crtend.o
OBJS+= crti.o crtn.o
#OBJS+= elf_note.o

.if ${MKPIC} == "yes"
OBJS+=		crtbeginS.o
OBJS+=		crtendS.o
CFLAGS.crtbegin.c+= -fPIE
CFLAGS.crtend.c+= -fPIE
.endif

realall: ${OBJS}

crtbegin.o: crtbegin.c csu_common.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crtbegin.c} ${COMMON_DIR}/crtbegin.c -o ${.TARGET}.o
		${COMPILE.c} ${CFLAGS.csu_common.c} ${COMMON_DIR}/csu_common.c -o ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

crtend.o: crtend.c csu_common.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crtend.c} ${COMMON_DIR}/crtend.c -o ${.TARGET}.o
		${COMPILE.c} ${CFLAGS.csu_common.c} ${COMMON_DIR}/csu_common.c -o ${.TARGET}.o
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

crti.o: crti.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${MY_PICFLAGS} ${COMMON_DIR}/crti.c -o ${.TARGET}.o
		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

crtn.o: crtn.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${MY_PICFLAGS} ${COMMON_DIR}/crtn.c -o ${.TARGET}.o
		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

crt0.o: crt0.c crt0_common.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${MY_PICFLAGS} ${ARCHDIR}/crt0.c -o ${.TARGET}.o
		${COMPILE.c} ${MY_PICFLAGS} ${COMMON_DIR}/crt0_common.c -o ${.TARGET}.o
		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

# dependant on crt0.o to pick up header dependencies
gcrt0.o: crt0.c crt0_common.c crt0.o
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${MY_PICFLAGS} -DMCRT0 ${ARCHDIR}/crt0.c -o ${.TARGET}.o
		${COMPILE.c} ${MY_PICFLAGS} -DMCRT0 ${COMMON_DIR}/crt0_common.c -o ${.TARGET}.o
		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

#elf_note.o: elf_note.S
#		${_MKTARGET_COMPILE}
#		${COMPILE.S} ${PICFLAGS} ${COMMON_DIR}/elf_note.S -o ${.TARGET}.o
#		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
#		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
#		rm -f ${.TARGET}.o
#.if ${MKSTRIPIDENT} != "no"
#		${OBJCOPY} -R .ident ${.TARGET}
#.endif

FILES=${OBJS}
FILESDIR=${LIBDIR}
CLEANFILES+=${OBJS}

.if ${MKPIC} == "yes"
SYMLINKS+=	crtbegin.o ${LIBDIR}/crtbeginT.o
.endif

.include <bsd.prog.mk>
