#	$211BSD: Makefile,v 1.0 2025/03/31 19:15:27 Exp $

WARNS?=	2

NOMAN=	# defined
BINDIR=/usr/bin

.include <bsd.own.mk>

USE_FORT?= yes	# cryptographic software

PROG=	openssl

LCRYPTO_SRC= ${LIBRESSLSRC}/crypto
LCRYPTO_INC= ${LIBRESSLSRC}/include
LAPPS_SRC= ${LIBRESSLSRC}/apps

CPPFLAGS+= -DLIBRESSL_INTERNAL
CPPFLAGS+= -I${LIBRESSLSRC}
CPPFLAGS+= -I${LCRYPTO_INC}
CPPFLAGS+= -I${LCRYPTO_INC}/compat
CPPFLAGS+= -I${LCRYPTO_SRC}
CPPFLAGS+= -I${LCRYPTO_SRC}/compat
CPPFLAGS+= -I${LAPPS_SRC}
CPPFLAGS+= -I${LAPPS_SRC}/openssl

LDADD+=	-lssl -lcrypto -lcrypt
DPADD+=	${LIBSSL} ${LIBCRYPTO} ${LIBCRYPT}

CRYPTODIST=	${NETBSDSRCDIR}/crypto

.include "${NETBSDSRCDIR}/crypto/Makefile.libressl"
.PATH:  ${LCRYPTO_SRC}/compat
.PATH:	${LAPPS_SRC}
.PATH:	${LAPPS_SRC}/openssl

# Compat
#SRCS += strtonum.c

# This list is built from the contents of the dist/apps/openssl/Makefile.am
# in the LibreSSL source distribution.

SRCS += apps.c
SRCS += asn1pars.c
SRCS += ca.c
SRCS += ciphers.c
SRCS += crl.c
SRCS += crl2p7.c
SRCS += cms.c
SRCS += dgst.c
SRCS += dh.c
SRCS += dhparam.c
SRCS += dsa.c
SRCS += dsaparam.c
SRCS += ec.c
SRCS += ecparam.c
SRCS += enc.c
SRCS += errstr.c
SRCS += gendh.c
SRCS += gendsa.c
SRCS += genpkey.c
SRCS += genrsa.c
SRCS += ocsp.c
SRCS += openssl.c
SRCS += passwd.c
SRCS += pkcs12.c
SRCS += pkcs7.c
SRCS += pkcs8.c
SRCS += pkey.c
SRCS += pkeyparam.c
SRCS += pkeyutl.c
SRCS += prime.c
SRCS += rand.c
SRCS += req.c
SRCS += rsa.c
SRCS += rsautl.c
SRCS += s_cb.c
SRCS += s_client.c
SRCS += s_server.c
SRCS += s_socket.c
SRCS += s_time.c
SRCS += sess_id.c
SRCS += smime.c
SRCS += speed.c
SRCS += ts.c
SRCS += verify.c
SRCS += version.c
SRCS += x509.c
SRCS += certhash.c

.include <bsd.prog.mk>
