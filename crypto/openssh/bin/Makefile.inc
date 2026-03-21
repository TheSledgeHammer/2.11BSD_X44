#	$NetBSD: Makefile.inc,v 1.3 2014/10/19 16:30:58 christos Exp $

# libnetbsd required for statvfs and fstatvfs
PROGDPLIBS+=netbsd  ${NETBSDSRCDIR}/contrib/oscompat/netbsd \
            ssh     ${NETBSDSRCDIR}/crypto/openssh/lib \
            crypto	${NETBSDSRCDIR}/crypto/${EXTERNAL_LIBRESSL_SUBDIR}/lib/libcrypto \
			crypt	${NETBSDSRCDIR}/usr.lib/libcrypt \
            util    ${NETBSDSRCDIR}/usr.lib/libutil \
			z		${NETBSDSRCDIR}/usr.lib/libz

.include "${.PARSEDIR}/../Makefile.inc"
