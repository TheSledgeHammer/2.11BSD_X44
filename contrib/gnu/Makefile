#	$211BSD: Makefile,v 1.0 2021/06/07 23:59:27 Exp $

.include <bsd.own.mk>

.if ${MKBSDGREP} == "no"
SUBDIR+=	grep
.endif

.if ${MKGROFF} != "no"
SUBDIR+=    groff
.endif

.if ${MKBINUTILS} != "no"
SUBDIR+=	${EXTERNAL_BINUTILS_SUBDIR}
.endif

.if ${MKGCC} != "no"
SUBDIR+=	${EXTERNAL_GCC_SUBDIR}
.endif

.if ${MKGDB} != "no"
SUBDIR+=    ${EXTERNAL_GDB_SUBDIR}
.endif

SUBDIR+= gettext diffutils texinfo rcs

.include <bsd.subdir.mk>
