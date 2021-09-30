#	$NetBSD: Makefile,v 1.22 2020/11/12 22:28:02 maya Exp $

SUBDIR+=  am-utils byacc compiler_rt cron elftoolchain/common   \
          file flex libarchive mdocml nawk xz

.if (${MKLLVM} != "no" || ${MKLLVMRT} != "no")
SUBDIR+= llvm
.endif

.if (${MKLIBCXX} != "no")
SUBDIR+= libc++
.endif

.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif

SUBDIR+= external

.include <bsd.subdir.mk>
