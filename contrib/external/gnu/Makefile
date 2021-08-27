#	$211BSD: Makefile,v 1.0 2021/06/07 23:59:27 Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no"
SUBDIR+=	${EXTERNAL_BINUTILS_SUBDIR}
.endif

SUBDIR+= autoconf gmp mpc mpfr texinfo

.include <bsd.subdir.mk>