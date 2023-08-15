#	$NetBSD: Makefile,v 1.36 2013/08/05 14:41:57 reinoud Exp $
#

.include <bsd.own.mk>

PROG=	makefs
SRCS=   ffs.c #cd9660.c 
SRCS+=	getid.c makefs.c misc.c pack_dev.c spec.c walk.c 
MAN=	makefs.8

MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree

CPPFLAGS+=	-I${.CURDIR} -I${MKNODSRC} -I${MTREESRC}
#CPPFLAGS+=	-DMSDOSFS_DEBUG
.PATH:		${MKNODSRC} ${MTREESRC}

#.include "${.CURDIR}/cd9660/Makefile.inc"
.include "${.CURDIR}/ffs/Makefile.inc"
#.include "${.CURDIR}/msdos/Makefile.inc"

.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

WARNS?=	3

.include <bsd.prog.mk>
