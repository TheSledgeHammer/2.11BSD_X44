#	$NetBSD: bsd.sys.mk,v 1.3 1996/05/15 19:46:51 scottr Exp $
#
# Overrides used for NetBSD source tree builds.

CFLAGS+= -Werror

.if defined(DESTDIR)
CFLAGS+= -nostdinc -idirafter ${DESTDIR}/usr/include
.endif

# Lex
.l:
		${LEX.l} -o${.TARGET:R}.yy.c ${.IMPSRC}
		${LINK.c} -o ${.TARGET} ${.TARGET:R}.yy.c ${LDLIBS} -ll
		rm -f ${.TARGET:R}.yy.c
.l.c:
		${LEX.l} -o${.TARGET} ${.IMPSRC}
.l.o:
		${LEX.l} -o${.TARGET:R}.yy.c ${.IMPSRC}
		${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.yy.c 
		rm -f ${.TARGET:R}.yy.c

# Yacc
.y:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		${LINK.c} -o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
		rm -f ${.TARGET:R}.tab.c
.y.c:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		mv ${.TARGET:R}.tab.c ${.TARGET}
.y.o:
		${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
		${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
		rm -f ${.TARGET:R}.tab.c