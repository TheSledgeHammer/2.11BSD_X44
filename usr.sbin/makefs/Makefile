#	$FreeBSD$

.include <bsd.own.mk>

PROG=	makefs
SRCS=	cd9660.c ffs.c makefs.c msdos.c mtree.c walk.c
MAN=	makefs.8

MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree

CPPFLAGS+=	-I${.CURDIR} -I${MKNODSRC} -I${MTREESRC} -DMAKEFS
.PATH:		${MKNODSRC} ${MTREESRC}

.include "${.CURDIR}/cd9660/Makefile.inc"
.include "${.CURDIR}/ffs/Makefile.inc"
.include "${.CURDIR}/msdos/Makefile.inc"

.if defined(HOSTPROG)
LDADD+=	-lm
.else
DPADD+= ${LIBUTIL} ${LIBM}
LDADD+= -lutil -lm

COPTS.cd9660.c+=		${CC_WNO_STRINGOP_TRUNCATION} ${CC_WNO_FORMAT_TRUNCATION}
COPTS.cd9660_conversion.c+=	${CC_WNO_FORMAT_TRUNCATION}
.endif

.include <bsd.prog.mk>
