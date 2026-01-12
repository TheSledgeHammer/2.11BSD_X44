#	$211BSD_X44: Makefile.inc,v 1.14 2025/05/26 17:33:59 martin Exp $
#	$NetBSD: Makefile.inc,v 1.14 2018/04/06 18:58:59 christos Exp $

WARNS?=	2

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server

SSHDIR?= ${NETBSDSRCDIR}/crypto/openssh
SSHDIST?= ${SSHDIR}/dist

OPENBSD_COMPAT?= ${SSHDIST}/openbsd-compat

CPPFLAGS+=-I${SSHDIST}
CPPFLAGS+=-I${SSHDIR}/include

.if ${HAVE_OPENSSL} < 11
CPPFLAGS+=-DOPENSSL_API_COMPAT=0x10100000L
.endif

.PATH: ${SSHDIST}
.PATH: ${SSHDIR}/include

CPPFLAGS+=-DLIBWRAP -D__SELECT_DECLARED

LINTFLAGS+=	-X 117	# nonportable '>>' on signed integer type
LINTFLAGS+=	-X 231	# parameter unused
LINTFLAGS+=	-X 247	# pointer cast to unrelated type
LINTFLAGS+=	-X 351	# missing header declaration

# We install libssh in /usr/lib/private so applications don't
# accidentally link against it.  Set LIBSUBDIR so libssh.so.N and
# libssh.so.N.M will be installed there there, and set PROGDPSUBDIRS so
# ssh/sshd/sshd-session/&c. will find it at link-time.
LIBSUBDIR=	private
PROGDPSUBDIRS+=	private
