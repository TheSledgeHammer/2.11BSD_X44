#	$NetBSD: Makefile,v 1.22 2020/11/12 22:28:02 maya Exp $

SUBDIR+= external
SUBDIR+= byacc compiler_rt file flex mdocml nawk xz

.if (${MKLLVM} != "no" || ${MKLLVMRT} != "no")
SUBDIR+= llvm
.endif

.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif

.include <bsd.subdir.mk>