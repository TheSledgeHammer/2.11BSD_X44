#	$NetBSD: Makefile,v 1.22 2020/11/12 22:28:02 maya Exp $

.include <bsd.own.mk>

SUBDIR= bc byacc expat fetch file flex gnu libevent 	\
	less libarchive liblzf lua mdocml nawk ntp 		\
	sqlite xz

SUBDIR+= pkg_install

.if (${MKLDAP} != "no")
SUBDIR+= openldap
.endif

.if (${MKLIBCXX} != "no")
SUBDIR+= libc++
.endif

.if (${MKPAM} != "no")
SUBDIR+= pam-u2f
.endif

.if (${MKPCC} != "no")
SUBDIR+= pcc
.endif

.if (${MKUNBOUND} != "no")
SUBDIR+= unbound
.endif

SUBDIR+= nvi
#SUBDIR+= oscompat

.include <bsd.subdir.mk>
