#	$NetBSD: Makefile,v 1.14 2018/02/04 16:22:48 christos Exp $

.include <bsd.own.mk>

#
# Change MKOPENSSL below for alternative Crypto libraries 
# For OpenSSL:  MKOPENSSL? = yes
# For LibreSSL: MKOPENSSL? = no
# 
MKOPENSSL?= yes

.if ${MKOPENSSL:Uno} != "no"
SUBDIR+= ${EXTERNAL_OPENSSL_SUBDIR}
.else
SUBDIR+= ${EXTERNAL_LIBRESSL_SUBDIR}
.endif
SUBDIR+= .WAIT openssh

.include <bsd.subdir.mk>
