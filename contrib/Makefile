#	$NetBSD: Makefile,v 1.22 2020/11/12 22:28:02 maya Exp $

SUBDIR=  	byacc elftoolchain/common file flex gnu 	\
		libarchive lua mdocml nawk oscompat xz

.include <bsd.own.mk>

#.if ${MKINGRES} != "no"
#SUBDIR+= ingres
#.endif

.if ${MKLIBCXX} != "no"
SUBDIR+= libc++
.endif

.if ${MKPCC} != "no"
SUBDIR+= pcc
.endif

.include <bsd.subdir.mk>
