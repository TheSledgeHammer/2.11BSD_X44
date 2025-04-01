#	$211BSD: Makefile,v 1.0 2025/03/31 19:15:27 Exp $
# 	Makefile LibreSSL 4.0 libssl

.include <bsd.own.mk>

LIB=	ssl
USE_FORT?=yes	# Secure Sockets Layer and cryptography libraries

WARNS=0

LCRYPTO_SRC=  	${LIBRESSLSRC}/crypto
LCRYPTO_INC=  	${LIBRESSLSRC}/include
LSSL_SRC=		${LIBRESSLSRC}/ssl

CPPFLAGS+= -DLIBRESSL_INTERNAL -DLIBRESSL_NAMESPACE
CPPFLAGS+= -Dlib${LIB}
CPPFLAGS+= -I${LIBRESSLSRC}
CPPFLAGS+= -I${LCRYPTO_SRC}
CPPFLAGS+= -I${LCRYPTO_SRC}/arch
CPPFLAGS+= -I${LCRYPTO_SRC}/bio
CPPFLAGS+= -I${LSSL_SRC}
CPPFLAGS+= -I${LSSL_SRC}/hidden
CPPFLAGS+= -I${LSSL_SRC}/hidden/openssl

CRYPTODIST=	${NETBSDSRCDIR}/crypto

.include "${NETBSDSRCDIR}/crypto/Makefile.libressl"
.PATH: ${LIBRESSLSRC}
.PATH: ${LCRYPTO_SRC}
.PATH: ${LCRYPTO_SRC}/arch
.PATH: ${LCRYPTO_SRC}/bio
.PATH: ${LSSL_SRC}
.PATH: ${LSSL_SRC}/hidden
.PATH: ${LSSL_SRC}/hidden/openssl

.include "Makefile.incs"

LIBDPLIBS+=crypto ${.CURDIR}/../libcrypto

AFLAGS+=-DELF

# This list is built from the contents of the dist/ssl/Makefile.am
# in the LibreSSL source distribution.

#INCS += bytestring.h
#INCS += srtp.h
#INCS += dtls_local.h
#INCS += ssl_local.h
#INCS += ssl_sigalgs.h
#INCS += ssl_tlsext.h
#INCS += tls_content.h
#INCS += tls_internal.h
#INCS += tls12_internal.h
#INCS += tls13_internal.h
#INCS += tls13_handshake.h
#INCS += tls13_record.h

SRCS += bio_ssl.c
SRCS += d1_both.c
SRCS += d1_lib.c
SRCS += d1_pkt.c
SRCS += d1_srtp.c
SRCS += pqueue.c
SRCS += s3_cbc.c
SRCS += s3_lib.c
SRCS += ssl_asn1.c
SRCS += ssl_both.c
SRCS += ssl_cert.c
SRCS += ssl_ciph.c
SRCS += ssl_ciphers.c
SRCS += ssl_clnt.c
SRCS += ssl_err.c
SRCS += ssl_init.c
SRCS += ssl_kex.c
SRCS += ssl_lib.c
SRCS += ssl_methods.c
SRCS += ssl_packet.c
SRCS += ssl_pkt.c
SRCS += ssl_rsa.c
SRCS += ssl_seclevel.c
SRCS += ssl_sess.c
SRCS += ssl_sigalgs.c
SRCS += ssl_srvr.c
SRCS += ssl_stat.c
SRCS += ssl_tlsext.c
SRCS += ssl_transcript.c
SRCS += ssl_txt.c
SRCS += ssl_versions.c
SRCS += t1_enc.c
SRCS += t1_lib.c
SRCS += tls_buffer.c
SRCS += tls_content.c
SRCS += tls_key_share.c
SRCS += tls_lib.c
SRCS += tls12_key_schedule.c
SRCS += tls12_lib.c
SRCS += tls12_record_layer.c
SRCS += tls13_client.c
SRCS += tls13_error.c
SRCS += tls13_handshake.c
SRCS += tls13_handshake_msg.c
SRCS += tls13_key_schedule.c
SRCS += tls13_legacy.c
SRCS += tls13_lib.c
SRCS += tls13_quic.c
SRCS += tls13_record.c
SRCS += tls13_record_layer.c
SRCS += tls13_server.c

#INCSDIR=/usr/include/openssl

PKGCONFIG= libssl openssl
.include "${.CURDIR}/../../pkgconfig.mk"

.include <bsd.lib.mk>
