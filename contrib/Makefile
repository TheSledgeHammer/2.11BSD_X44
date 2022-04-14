#	$NetBSD: Makefile,v 1.22 2020/11/12 22:28:02 maya Exp $

SUBDIR=  	byacc elftoolchain/common file flex gnu 	\
			libarchive mdocml nawk oscompat xz

.include <bsd.own.mk>

#.if ${MKINGRES} != "no"
#SUBDIR+= ingres
#.endif

.if ${MKLIBCXX} != "no"
SUBDIR+= libc++
.endif

.if ${MKLLVM} != "no" || ${MKLLVMRT} != "no"
SUBDIR+= llvm
.endif

.if ${MKPCC} != "no"
SUBDIR+= pcc
.endif

.include <bsd.subdir.mk>
