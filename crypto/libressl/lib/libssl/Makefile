
.include <bsd.own.mk>

USE_FORT?=yes	# cryptographic software and network library

LIB=	ssl
CPPFLAGS+= -Dlib${LIB} -I${LIBRESSLSRC} -I${LIBRESSLSRC}/crypto
CPPFLAGS+= -I${LIBRESSLSRC}/include

CRYPTODIST=	${NETBSDSRCDIR}/crypto

.include "${NETBSDSRCDIR}/crypto/Makefile.libressl"
.PATH: ${LIBRESSLSRC} ${LIBRESSLSRC}/include/openssl
.PATH: ${LIBRESSLSRC}/ssl

INCS += bytestring.h
INCS += srtp.h
INCS += dtls_local.h
INCS += ssl_local.h
INCS += ssl_sigalgs.h
INCS += ssl_tlsext.h
INCS += tls_content.h
INCS += tls_internal.h
INCS += tls12_internal.h
INCS += tls13_internal.h
INCS += tls13_handshake.h
INCS += tls13_record.h

INCSDIR=/usr/include/openssl

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

PKGCONFIG=libssl
.include "${.CURDIR}/../../pkgconfig.mk"

.include <bsd.lib.mk>
