#	$NetBSD: Makefile,v 1.22 2020/11/12 22:28:02 maya Exp $

.include <bsd.own.mk>
SUBDIR= byacc expat fetch file flex gnu \
		less libarchive liblzf lua mdocml nawk xz

#SUBDIR+=	oscompat
SUBDIR+=	pkg_install

.if ${MKLIBCXX} != "no"
SUBDIR+= libc++
.endif

.if ${MKPCC} != "no"
SUBDIR+= pcc
.endif

.include <bsd.subdir.mk>
