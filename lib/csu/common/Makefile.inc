#	$NetBSD: Makefile.inc,v 1.35 2018/12/28 18:17:11 christos Exp $

.include <bsd.own.mk>

COMMON_DIR:=	${.CURDIR}/common
.PATH:			${COMMON_DIR}

CPPFLAGS+=		-I${COMMON_DIR} -I.

SRCS+=	crt0.c
OBJS+=	crt0.o gcrt0.o crtbegin.o crtend.o crti.o crtn.o
OBJS+=	init_note.o

realall: ${OBJS}

crt0.o: crt0.c crtbegin.c crtend.c crt.h init_note.c
		${CC} ${CFLAGS} -c -DCRT0 ${ARCHDIR}/${.ALLSRC}
		${LD} -x -r ${.TARGET}

gcrt0.o: crt0.c crtbegin.c crtend.c crt.h init_note.c
		${CC} ${CFLAGS} -c -DMCRT0 ${COMMON_DIR}/${.ALLSRC} -o ${.TARGET}
		${LD} -x -r ${.TARGET}

crtbegin.o: crtbegin.c
crtend.o: crtend.c
crti.o: crti.S
crtn.o: crtn.S
init_note.o: init_note.S note.h

FILES=${OBJS}
FILESDIR=${LIBDIR}
CLEANFILES+=${OBJS}

.include <bsd.prog.mk>
