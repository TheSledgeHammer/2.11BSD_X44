#	$NetBSD: Makefile,v 1.22 2020/11/12 22:28:02 maya Exp $

.include <bsd.own.mk>

SUBDIR= byacc expat fetch file flex gnu libevent 	\
	less liblzf lua mdocml nawk ntp sqlite

# Has alot of missing dependencies!
#SUBDIR+= libarchive 

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

.if (${MKXZ} != "no")
SUBDIR+= xz
.endif

SUBDIR+= nvi

.include <bsd.subdir.mk>

# Disabled: (Lack config options)
# - Unable to currently compile
# - bc, libarchive
