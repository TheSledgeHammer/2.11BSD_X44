#	$NetBSD: Makefile.inc,v 1.2 2021/05/30 01:56:59 joerg Exp $

.include <bsd.own.mk>

LIBCXXRT_SRCDIR:=	${.PARSEDIR}/dist/libcxxrt
LIBCXX_SRCDIR:=		${.PARSEDIR}/dist/libcxx
# LLVM Not currently in source tree
#LIBCXX_SRCDIR:=	${NETBSDSRCDIR}/contrib/llvm/dist/libcxx