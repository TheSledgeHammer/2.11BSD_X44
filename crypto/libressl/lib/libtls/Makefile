
.include <bsd.own.mk>

USE_FORT?=yes	# cryptographic software and network library

LIB=	tls
CPPFLAGS+= -Dlib${LIB} -I${LIBRESSLSRC} -I${LIBRESSLSRC}/crypto
CPPFLAGS+= -I${LIBRESSLSRC}/include

CRYPTODIST=	${NETBSDSRCDIR}/crypto

.include "${NETBSDSRCDIR}/crypto/Makefile.libressl"
.PATH: ${LIBRESSLSRC} ${LIBRESSLSRC}/include/openssl
.PATH: ${LIBRESSLSRC}/tls

INCS = tls_internal.h
INCSDIR=/usr/include/openssl

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

PKGCONFIG=libtls
.include "${.CURDIR}/../../pkgconfig.mk"

.include <bsd.lib.mk>
