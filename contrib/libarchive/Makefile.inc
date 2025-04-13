# $NetBSD: Makefile.inc,v 1.3 2017/04/20 20:34:24 joerg Exp $

.include <bsd.own.mk>

USE_FORT?=	yes # complex string handling

LIBARCHIVEDIR=	${NETBSDSRCDIR}/contrib/libarchive/dist

CPPFLAGS+=	-I${NETBSDSRCDIR}/contrib/libarchive/include
CPPFLAGS+=	-DPLATFORM_CONFIG_H=\"config_netbsd.h\"

.PATH:      ${NETBSDSRCDIR}/include/hash
CPPFLAGS+=  -I${NETBSDSRCDIR}/include/hash

LIBARCHIVE_FE_DIR!=	cd ${.PARSEDIR}/lib/libarchive_fe && ${PRINTOBJDIR}
LIBARCHIVE_FE=		${LIBARCHIVE_FE_DIR}/libarchive_fe.a

# GCC applies the check even for same functions like strftime,
# completely defeating the purpose.
CWARNFLAGS.gcc+=	-Wno-format-nonliteral

WARNS?=	4
