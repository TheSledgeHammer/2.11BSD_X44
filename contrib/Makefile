#	$NetBSD: Makefile,v 1.22 2020/11/12 22:28:02 maya Exp $

.include <bsd.own.mk>
SUBDIR=  	byacc expat fetch file flex gnu \
		libarchive lua mdocml nawk xz
#SUBDIR+=	oscompat

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
