# $NetBSD: pkgconfig.mk,v 1.6 2014/07/07 09:14:43 joerg Exp $

.include <bsd.own.mk>

FILESDIR=/usr/lib/pkgconfig
.for pkg in ${PKGCONFIG}
FILES+=${pkg}.pc
FILESBUILD_${pkg}.pc=yes

${pkg}.pc: ${.CURDIR}/../../mkpc
	CPPFLAGS=${CPPFLAGS:N-DLIBRESSLSRC=*:N-DENGINESDIR=*:Q} CPP=${CPP:Q} ${HOST_SH} ${.ALLSRC} ${LIBRESSLSRC}/crypto ${.TARGET} > ${.TARGET}
.endfor
