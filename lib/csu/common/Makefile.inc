#	$NetBSD: Makefile.inc,v 1.35 2018/12/28 18:17:11 christos Exp $

.include <bsd.own.mk>

# XXX: FIXME: This is defined in bsd.lib.mk
PICFLAGS ?= -fPIC

COMMON_DIR:=	${.CURDIR}/common
.PATH:			${COMMON_DIR}

CPPFLAGS+=		-I${COMMON_DIR} -I.

SRCS+=	crt0.c init_note.c crtbegin.c crtend.c
OBJS+=	crt0.o gcrt0.o crtbegin.o crtend.o crti.o crtn.o
OBJS+=	init_note.o

.if ${MKPIC} == "yes"
CFLAGS.crtbegin.c+= -fPIE
.endif

realall: ${OBJS}

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

crt0.o: crt0.c
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${CFLAGS.crt0.c} ${MY_PICFLAGS} ${ARCHDIR}/crt0.c -o ${.TARGET}.o
		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

# dependant on crt0.o to pick up header dependencies
gcrt0.o: crt0.c crt0.o
		${_MKTARGET_COMPILE}
		${COMPILE.c} ${MY_PICFLAGS} -DMCRT0 ${ARCHDIR}/crt0.c -o ${.TARGET}.o
		${LD} -x -r -o ${.TARGET} ${.TARGET}.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.o
.if ${MKSTRIPIDENT} != "no"
		${OBJCOPY} -R .ident ${.TARGET}
.endif

init_note.o: init_note.S init_note.c
		${_MKTARGET_COMPILE}
		${COMPILE.S} ${COMMON_DIR}/init_note.S -o ${.TARGET}.S.o
		${COMPILE.c} ${CFLAGS.init_note.c} ${COMMON_DIR}/init_note.c -o ${.TARGET}c.o
		${LD} -r -o ${.TARGET}.o ${.TARGET}.S.o ${.TARGET}.c.o
		${OBJCOPY} ${OBJCOPYLIBFLAGS} ${.TARGET}.o ${.TARGET}
		rm -f ${.TARGET}.S.o ${.TARGET}.c.o ${.TARGET}.o

crti.o: crti.S
crtn.o: crtn.S
init_note.o: init_note.S note.h

FILES=${OBJS}
FILESDIR=${LIBDIR}
CLEANFILES+=${OBJS}

.include <bsd.prog.mk>
