#	$211BSD: Makefile,v 1.0 2025/03/31 19:15:27 Exp $
# 	Makefile LibreSSL 4.0 libtls

.include <bsd.own.mk>

LIB=	tls
USE_FORT?=yes	# Secure communications using the TLS socket protocol

WARNS=0

LCRYPTO_SRC=  	${LIBRESSLSRC}/crypto
LCRYPTO_INC=  	${LIBRESSLSRC}/include
LTLS_SRC=		${LIBRESSLSRC}/tls

CPPFLAGS+= -Dlib${LIB}
CPPFLAGS+= -I${LIBRESSLSRC}
CPPFLAGS+= -I${LCRYPTO_INC}
CPPFLAGS+= -I${LCRYPTO_INC}/openssl
CPPFLAGS+= -I${LCRYPTO_SRC}

CRYPTODIST=	${NETBSDSRCDIR}/crypto

.include "${NETBSDSRCDIR}/crypto/Makefile.libressl"
.PATH: ${LIBRESSLSRC}
.PATH: ${LCRYPTO_INC}
.PATH: ${LCRYPTO_INC}/openssl
.PATH: ${LCRYPTO_SRC}

LIBDPLIBS+=crypto ${.CURDIR}/../libcrypto
LIBDPLIBS+=ssl ${.CURDIR}/../libssl

AFLAGS+=-DELF

# This list is built from the contents of the dist/tls/Makefile.am
# in the LibreSSL source distribution.

#INCS = tls_internal.h

SRCS += tls.c
SRCS += tls_client.c
SRCS += tls_bio_cb.c
SRCS += tls_config.c
SRCS += tls_conninfo.c
SRCS += tls_keypair.c
SRCS += tls_server.c
SRCS += tls_signer.c
SRCS += tls_ocsp.c
SRCS += tls_peer.c
SRCS += tls_util.c
SRCS += tls_verify.c

#INCSDIR=/usr/include/openssl

PKGCONFIG= libtls openssl
.include "${.CURDIR}/../../pkgconfig.mk"

.include <bsd.lib.mk>
