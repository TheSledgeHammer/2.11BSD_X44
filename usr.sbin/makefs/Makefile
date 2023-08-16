#	$NetBSD: Makefile,v 1.36 2013/08/05 14:41:57 reinoud Exp $
#

.include <bsd.own.mk>

PROG=	makefs
SRCS=   cd9660.c ffs.c
SRCS+=	getid.c makefs.c misc.c pack_dev.c spec.c walk.c 
MAN=	makefs.8

MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree

CPPFLAGS+=	-I${.CURDIR} -I${MKNODSRC} -I${MTREESRC}
#CPPFLAGS+=	-DMSDOSFS_DEBUG
.PATH:		${MKNODSRC} ${MTREESRC}

.include "${.CURDIR}/cd9660/Makefile.inc"
.include "${.CURDIR}/ffs/Makefile.inc"
#.include "${.CURDIR}/msdos/Makefile.inc"

.if defined(HOSTPROG)
LDADD+=	-lm
.else
DPADD+= ${LIBUTIL} ${LIBM}
LDADD+= -lutil -lm

COPTS.cd9660.c+=		${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_FORMAT_TRUNCATION}
COPTS.cd9660_conversion.c+=	${GCC_NO_FORMAT_TRUNCATION}
.endif

WARNS?=5

.include <bsd.prog.mk>
