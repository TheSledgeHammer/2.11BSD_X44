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

.if ${MKPOSTFIX} != "no"
SUBDIR+=	postfix
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-postfix:
	@true

.include <bsd.subdir.mk>
